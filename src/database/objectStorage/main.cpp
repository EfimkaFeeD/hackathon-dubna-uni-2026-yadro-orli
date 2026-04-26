#include <crow.h>
#include "objectStorage.h"
#include <memory>
#include <iostream>
#include <string>
#include <vector>

std::unique_ptr<objectStorage> g_storage;

int main() {
    g_storage = std::make_unique<objectStorage>("../data/objects", 1024 * 1024 * 500);

    std::cout << "Object Storage Service Started on port 8081" << std::endl;
    std::cout << "============================================" << std::endl;

    crow::SimpleApp app;

    CROW_ROUTE(app, "/health")
            ([]() {
                return crow::response(200, "OK");
            });

    // PUT - save chunk
    CROW_ROUTE(app, "/v1/objects/put/<int>/<string>")
            ([&](const crow::request& req, crow::response& res, int fileId, std::string key) {
                if (req.body.empty()) {
                    res.code = 400;
                    res.write("Empty body");
                    return;
                }

                std::vector<std::byte> data;
                data.reserve(req.body.size());
                for (char c : req.body) {
                    data.push_back(static_cast<std::byte>(c));
                }

                if (g_storage->save(static_cast<size_t>(fileId), key, data)) {
                    res.code = 201;
                    res.set_header("Location", "/v1/objects/get/" + std::to_string(fileId) + "/" + key);
                } else {
                    res.code = 500;
                    res.write("Failed to save");
                }
            });

    // GET - load chunk
    CROW_ROUTE(app, "/v1/objects/get/<int>/<string>")
            ([&](const crow::request& req, crow::response& res, int fileId, std::string key) {
                auto data = g_storage->load(static_cast<size_t>(fileId), key);
                if (!data.has_value()) {
                    res.code = 404;
                    res.write("Object not found");
                    return;
                }

                res.code = 200;
                res.set_header("Content-Type", "application/octet-stream");
                res.body.assign(reinterpret_cast<const char*>(data->data()), data->size());
            });

    // DELETE - delete one chunk
    CROW_ROUTE(app, "/v1/objects/delete/<int>/<string>")
            ([&](const crow::request& req, crow::response& res, int fileId, std::string key) {
                if (g_storage->remove(static_cast<size_t>(fileId), key)) {
                    res.code = 204;
                } else {
                    res.code = 404;
                    res.write("Object not found");
                }
            });

    // DELETE - delete all chunks of file
    CROW_ROUTE(app, "/v1/objects/all/<int>")
            ([&](const crow::request& req, crow::response& res, int fileId) {
                if (g_storage->removeAllForFile(static_cast<size_t>(fileId))) {
                    res.code = 204;
                } else {
                    res.code = 404;
                    res.write("File not found");
                }
            });

    // GET - list of all chunks of file
    CROW_ROUTE(app, "/v1/objects/list/<int>")
            ([&](const crow::request& req, crow::response& res, int fileId) {
                auto keys = g_storage->listKeysForFile(static_cast<size_t>(fileId));
                auto totalSize = g_storage->getFileSize(static_cast<size_t>(fileId));

                std::string result = "{\"file_id\": " + std::to_string(fileId) +
                                     ", \"chunks_count\": " + std::to_string(keys.size()) +
                                     ", \"total_size\": " + std::to_string(totalSize) +
                                     ", \"chunks\": [";
                for (size_t i = 0; i < keys.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += "\"" + keys[i] + "\"";
                }
                result += "]}";

                res.code = 200;
                res.set_header("Content-Type", "application/json");
                res.write(result);
            });

    // GET - list of all file_id
    CROW_ROUTE(app, "/v1/objects/all")
            ([&](const crow::request& req, crow::response& res) {
                auto ids = g_storage->listAllFileIds();

                std::string result = "{\"total_files\": " + std::to_string(ids.size()) +
                                     ", \"file_ids\": [";
                for (size_t i = 0; i < ids.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += std::to_string(ids[i]);
                }
                result += "]}";

                res.code = 200;
                res.set_header("Content-Type", "application/json");
                res.write(result);
            });

    std::cout << "Endpoints:" << std::endl;
    std::cout << "  PUT    /v1/objects/put/{file_id}/{key}     - upload chunk" << std::endl;
    std::cout << "  GET    /v1/objects/get/{file_id}/{key}     - download chunk" << std::endl;
    std::cout << "  DELETE /v1/objects/delete/{file_id}/{key}  - delete chunk" << std::endl;
    std::cout << "  DELETE /v1/objects/all/{file_id}           - delete all chunks" << std::endl;
    std::cout << "  GET    /v1/objects/list/{file_id}          - list chunks" << std::endl;
    std::cout << "  GET    /v1/objects/all                     - list all files" << std::endl;
    std::cout << "  GET    /health                             - health check" << std::endl;
    std::cout << "============================================" << std::endl;

    app.port(8081).multithreaded().run();

    return 0;
}

/*
 * 	http://localhost:8081
 *  HTTP
 *  Binary (application/octet-stream)
 *  UTF-8 for metadata
 *
 *  1. Save chunk
 *  PUT /v1/objects/put/{file_id}/{key}
    Content-Type: application/octet-stream
    <binary data of chunk>
 *
 *  bash
 *  curl -X PUT -d "Hello World" http://localhost:8081/v1/objects/put/123/chunk_001
 *
 *  201 Created	(success)
    400 Bad Request	(empty body of request)
    500 Internal Server Error
 *
 *
 *  2.  Load chunk
 *  GET /v1/objects/get/{file_id}/{key}
 *
 *  bash
 *  curl http://localhost:8081/v1/objects/get/123/chunk_001 --output chunk_001.bin
 *
 *  200 OK	(return binary data of chunk)
    404 Not Found (chunk not found)
 *
 *
 *  3.  Delete one chunk
 *  DELETE /v1/objects/delete/{file_id}/{key}
 *
 *  bash
 *  curl -X DELETE http://localhost:8081/v1/objects/delete/123/chunk_001
 *
 *  204 No Content (success)
    404 Not Found (chunk not found)
 *
 *
 *  4. Delete all chunks of file
 *
 *  DELETE /v1/objects/all/{file_id}
 *
 *  bash
 *  curl -X DELETE http://localhost:8081/v1/objects/all/123
 *
 *
 *  204 No Content (success)
    404 Not Found (file not found)
 *
 *
 *
 *  5. Get list of all chunks of file
 *
 *  GET /v1/objects/list/{file_id}
 *
 *  bash
 *  curl http://localhost:8081/v1/objects/list/123
 *
 *  example of data in body of answer
 *  {
        "file_id": 123,
        "chunks_count": 3,
        "total_size": 1024,
        "chunks": ["chunk_001", "chunk_002", "metadata"]
    }
 *
 *
 *
 *
 *  6. Get list of all files
 *
 *  GET /v1/objects/all
 *
 *  bash
 *  curl http://localhost:8081/v1/objects/all
 *
 *
 *  example of data in body of answer
 *  {
        "total_files": 2,
        "file_ids": [123, 456]
    }
 *
 *
 *  7. Ping
 *
 *  GET /health
 *
 *  bash
 *  curl http://localhost:8081/health
 *
 *  200 OK (Server is working)
 * */