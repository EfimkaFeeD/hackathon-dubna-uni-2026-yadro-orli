#ifndef BD_BD_H
#define BD_BD_H

#include <sqlite3.h>
#include <iostream>
#include <string>
#include "enums_and_structs.h"

struct expiredData {
    std::vector<size_t> fileIds;
    std::vector<std::string> chunkKeys;
};

struct userStruct {
    size_t id;
    std::string token;
    std::string data_finish;
};

struct fileStruct {
    size_t id;
    int bit;
    bool signed_;
    int sample_rate;
    size_t user_id;
    int channels;
    int format;
    std::string extension;
};

struct chunkStruct {
    size_t id;
    size_t file_id;
    std::string key;
    int chunk_type;
};

class dataBase {
    sqlite3* db;
    std::string buildColumn(const column& col);
    std::string generateCheckConstraint(const column& col);
    std::string columnToSQL(const column& col);
    std::string generateCreateTableSQL(const table& tbl);
public:
    dataBase();
    ~dataBase();
    bool createTable(const table& tbl);
    bool createAllTables(const std::vector<table> tables);

    size_t insertUser(const std::string& token, const std::string& dataFinish);
    size_t insertFile(int bit, int signedVal, int sampleRate, int channels, int format,
                       const std::string& extension, size_t userId);
    size_t insertChunk(size_t fileId, const std::string& key, int chunkType);

    std::optional<userStruct> getUserByToken(const std::string& token);
    std::optional<userStruct> getUserByUserId(size_t userId);
    std::vector<fileStruct> getFilesByUser(const std::string& token);
    std::optional<fileStruct> getFileByFileId(size_t fileId);
    std::vector<chunkStruct> getChunksByFile(size_t fileId);
    std::optional<chunkStruct> getChunkByChunkId(size_t chunkId);

    bool updateUserToken(size_t userId, const std::string& newToken);

    std::vector<std::string> getExpiredKeys();
    expiredData deleteExpiredUserData();
};
#endif //BD_BD_H
