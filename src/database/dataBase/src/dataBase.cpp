#include "dataBase.h"

dataBase::dataBase() {
    int errorCode = sqlite3_open("dataBase.db", &db);
    if (errorCode != SQLITE_OK) {
        std::cerr << "Cannot open database" << sqlite3_errmsg(db) << std::endl;
    }
}

dataBase::~dataBase() {
    sqlite3_close(db);
}

std::string dataBase::generateCheckConstraint(const column& col) {
    std::vector<std::string> conditions;

    if (col.minValue.has_value()) {
        conditions.push_back(col.name + " >= " + std::to_string(col.minValue.value()));
    }
    if (col.maxValue.has_value()) {
        conditions.push_back(col.name + " <= " + std::to_string(col.maxValue.value()));
    }

    if (col.minLength.has_value()) {
        conditions.push_back("LENGTH(" + col.name + ") >= " + std::to_string(col.minLength.value()));
    }
    if (col.maxLength.has_value()) {
        conditions.push_back("LENGTH(" + col.name + ") <= " + std::to_string(col.maxLength.value()));
    }

    if (!col.allowedValues.empty()) {
        std::string valuesList;
        for (size_t i = 0; i < col.allowedValues.size(); i++) {
            bool needsQuotes = (col.type == dbTypes::TEXT || col.type == dbTypes::BLOB);
            if (needsQuotes) {
                valuesList += "'" + col.allowedValues[i] + "'";
            } else {
                valuesList += col.allowedValues[i];
            }
            if (i != col.allowedValues.size() - 1) {
                valuesList += ", ";
            }
        }
        conditions.push_back(col.name + " IN (" + valuesList + ")");
    }

    if (conditions.empty()) {
        return "";
    }

    std::string check = "CHECK (";
    for (size_t i = 0; i < conditions.size(); ++i) {
        check += conditions[i];
        if (i != conditions.size() - 1) {
            check += " AND ";
        }
    }
    check += ")";

    return check;
}

std::string dataBase::columnToSQL(const column& col) {
    std::string sql = col.name + " " + typeToString(col.type);

    std::string consStr = constraintToString(col.cons);
    if (consStr != "NONE" && consStr != "REFERENCE ") {
        sql += " " + consStr;
    } else if (consStr == "REFERENCE ") {
        sql += " " + consStr + col.foreignTable + "(" + col.foreignColumn + ")";
    }

    std::string defaultStr = defaultValuesToString(col.readyDefaultValue);
    if (defaultStr != "NONE" && defaultStr != "CUSTOM_DEFAULT_VALUE") {
        sql += " " + defaultStr;
    } else if (defaultStr == "CUSTOM_DEFAULT_VALUE") {
        sql += " " + col.customDefaultValue;
    }

    return sql;
}

std::string dataBase::generateCreateTableSQL(const table& tbl) {
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tbl.name + " (\n";

    for (size_t i = 0; i < tbl.columns.size(); i++) {
        sql += "    " + columnToSQL(tbl.columns[i]);

        std::string check = generateCheckConstraint(tbl.columns[i]);
        if (!check.empty()) {
            sql += " " + check;
        }

        if (i != tbl.columns.size() - 1) {
            sql += ",";
        }
        sql += "\n";
    }

    if (sql.back() == '\n' && sql[sql.size() - 2] == ',') {
        sql.erase(sql.size() - 3, 2);
    }

    sql += ");";
    return sql;
}

bool dataBase::createTable(const table& tbl) {
    std::string sql = generateCreateTableSQL(tbl);
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
        std::cerr << "Failed to create table " + tbl.name + ": " + err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

bool dataBase::createAllTables(const std::vector<table> tables) {
    bool all_success = true;

    for (const auto& tbl : tables) {
        if (!createTable(tbl)) {
            all_success = false;
        }
    }
}

size_t dataBase::insertUser(const std::string &token, const std::string &dataFinish) {
    const char* sql = "INSERT INTO users (token, data_finish) VALUES (?, ?)";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, dataFinish.c_str(), -1, SQLITE_STATIC);

    sqlite3_step(stmt);
    size_t newId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    return newId;
}

size_t
dataBase::insertFile(int bit, int signedVal, int sampleRate, int channels, int format, const std::string &extension,
                     size_t userId) {
    const char* sql = "INSERT INTO files (bit, signed, sample_rate, channels, format, extension, user_id) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, bit);
    sqlite3_bind_int(stmt, 2, signedVal);
    sqlite3_bind_int(stmt, 3, sampleRate);
    sqlite3_bind_int(stmt, 4, channels);
    sqlite3_bind_int(stmt, 5, format);
    sqlite3_bind_text(stmt, 6, extension.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 7, userId);

    sqlite3_step(stmt);
    size_t newId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    return newId;
}

size_t dataBase::insertChunk(size_t fileId, const std::string &key, int chunkType) {
    const char* sql = "INSERT INTO chunks (file_id, key, chunk_type) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, fileId);
    sqlite3_bind_text(stmt, 2, key.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, chunkType);

    sqlite3_step(stmt);
    size_t newId = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);

    return newId;
}

std::vector<std::string> dataBase::getExpiredKeys() {
    std::vector<std::string> keys;

    const char* sql = R"(
        SELECT key FROM chunks
        WHERE file_id IN (
            SELECT file_id FROM files
            WHERE user_id IN (
                SELECT user_id FROM users
                WHERE data_finish <= datetime('now')
            )
        )
    )";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string key = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        if (!key.empty()) {
            keys.push_back(key);
        }
    }

    sqlite3_finalize(stmt);
    return keys;
}

std::optional<userStruct> dataBase::getUserByToken(const std::string& token) {
    const char* sql = "SELECT user_id, token, data_finish FROM users WHERE token = ?";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);

    std::optional<userStruct> result;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        userStruct user;
        user.id = sqlite3_column_int64(stmt, 0);
        user.token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.data_finish = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result = user;
    }

    sqlite3_finalize(stmt);
    return result;
}

std::vector<fileStruct> dataBase::getFilesByUser(const std::string &token) {
    std::vector<fileStruct> files;

    const char* userSql = "SELECT user_id FROM users WHERE token = ?";
    sqlite3_stmt* userStmt;

    sqlite3_prepare_v2(db, userSql, -1, &userStmt, nullptr);
    sqlite3_bind_text(userStmt, 1, token.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(userStmt) != SQLITE_ROW) {
        sqlite3_finalize(userStmt);
        return files;
    }

    size_t userId = sqlite3_column_int64(userStmt, 0);
    sqlite3_finalize(userStmt);

    const char* filesSql = R"(
        SELECT file_id, bit, signed, sample_rate, user_id, channels, format, extension
        FROM files
        WHERE user_id = ?
    )";

    sqlite3_stmt* fileStmt;
    sqlite3_prepare_v2(db, filesSql, -1, &fileStmt, nullptr);
    sqlite3_bind_int64(fileStmt, 1, userId);

    while (sqlite3_step(fileStmt) == SQLITE_ROW) {
        fileStruct file;
        file.id = sqlite3_column_int64(fileStmt, 0);
        file.bit = sqlite3_column_int(fileStmt, 1);
        file.signed_ = sqlite3_column_int(fileStmt, 2) != 0;
        file.sample_rate = sqlite3_column_int(fileStmt, 3);
        file.user_id = sqlite3_column_int64(fileStmt, 4);
        file.channels = sqlite3_column_int(fileStmt, 5);
        file.format = sqlite3_column_int(fileStmt, 6);
        file.extension = reinterpret_cast<const char*>(sqlite3_column_text(fileStmt, 7));

        files.push_back(file);
    }

    sqlite3_finalize(fileStmt);
    return files;
}

std::optional<fileStruct> dataBase::getFileByFileId(size_t fileId) {
    const char* sql = R"(
        SELECT file_id, bit, signed, sample_rate, user_id, channels, format, extension
        FROM files
        WHERE file_id = ?
    )";

    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_int64(stmt, 1, fileId);

    std::optional<fileStruct> result;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        fileStruct file;
        file.id = sqlite3_column_int64(stmt, 0);
        file.bit = sqlite3_column_int(stmt, 1);
        file.signed_ = sqlite3_column_int(stmt, 2) != 0;
        file.sample_rate = sqlite3_column_int(stmt, 3);
        file.user_id = sqlite3_column_int64(stmt, 4);
        file.channels = sqlite3_column_int(stmt, 5);
        file.format = sqlite3_column_int(stmt, 6);
        file.extension = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));

        result = file;
    }

    sqlite3_finalize(stmt);

    return result;
}

std::optional<userStruct> dataBase::getUserByUserId(size_t userId) {
    const char* sql = "SELECT user_id, token, data_finish FROM users WHERE user_id = ?";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_int64(stmt, 1, userId);

    std::optional<userStruct> result;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        userStruct user;
        user.id = sqlite3_column_int64(stmt, 0);
        user.token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.data_finish = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        result = user;
    }

    sqlite3_finalize(stmt);

    return result;
}

std::vector<chunkStruct> dataBase::getChunksByFile(size_t fileId) {
    const char* sql = "SELECT chunk_id, file_id, key, chunk_type FROM chunks WHERE file_id = ?";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_int64(stmt, 1, fileId);

    std::vector<chunkStruct> chunks;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        chunkStruct chunk;
        chunk.id = sqlite3_column_int64(stmt, 0);
        chunk.file_id = sqlite3_column_int64(stmt, 1);
        chunk.key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        chunk.chunk_type = sqlite3_column_int(stmt, 3);

        chunks.push_back(chunk);
    }

    sqlite3_finalize(stmt);

    return chunks;
}

std::optional<chunkStruct> dataBase::getChunkByChunkId(size_t chunkId) {
    const char* sql = "SELECT chunk_id, file_id, key, chunk_type FROM chunks WHERE chunk_id = ?";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_int64(stmt, 1, chunkId);

    std::optional<chunkStruct> result;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        chunkStruct chunk;
        chunk.id = sqlite3_column_int64(stmt, 0);
        chunk.file_id = sqlite3_column_int64(stmt, 1);
        chunk.key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        chunk.chunk_type = sqlite3_column_int(stmt, 3);

        result = chunk;
    }

    sqlite3_finalize(stmt);

    return result;
}

bool dataBase::updateUserToken(size_t userId, const std::string& newToken) {
    const char* sql = "UPDATE users SET token = ? WHERE user_id = ?";
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, newToken.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, userId);

    int rc = sqlite3_step(stmt);

    bool success = (rc == SQLITE_DONE);

    sqlite3_finalize(stmt);

    return success;
}

expiredData dataBase::deleteExpiredUserData() {
    expiredData result;

    const char* expiredUsersSql = R"(
        SELECT user_id FROM users
        WHERE data_finish <= datetime('now')
    )";

    sqlite3_stmt* userStmt;
    sqlite3_prepare_v2(db, expiredUsersSql, -1, &userStmt, nullptr);

    std::vector<size_t> expiredUserIds;
    while (sqlite3_step(userStmt) == SQLITE_ROW) {
        size_t userId = sqlite3_column_int64(userStmt, 0);
        expiredUserIds.push_back(userId);
    }
    sqlite3_finalize(userStmt);

    if (expiredUserIds.empty()) {
        return result;
    }

    const char* filesSql = "SELECT file_id FROM files WHERE user_id = ?";
    sqlite3_stmt* fileStmt;
    sqlite3_prepare_v2(db, filesSql, -1, &fileStmt, nullptr);

    for (size_t userId : expiredUserIds) {
        sqlite3_bind_int64(fileStmt, 1, userId);

        while (sqlite3_step(fileStmt) == SQLITE_ROW) {
            size_t fileId = sqlite3_column_int64(fileStmt, 0);
            result.fileIds.push_back(fileId);
        }

        sqlite3_reset(fileStmt);
    }
    sqlite3_finalize(fileStmt);

    const char* chunksSql = "SELECT key FROM chunks WHERE file_id = ?";
    sqlite3_stmt* chunkStmt;
    sqlite3_prepare_v2(db, chunksSql, -1, &chunkStmt, nullptr);

    for (size_t fileId : result.fileIds) {
        sqlite3_bind_int64(chunkStmt, 1, fileId);

        while (sqlite3_step(chunkStmt) == SQLITE_ROW) {
            std::string key = reinterpret_cast<const char*>(sqlite3_column_text(chunkStmt, 0));
            if (!key.empty()) {
                result.chunkKeys.push_back(key);
            }
        }

        sqlite3_reset(chunkStmt);
    }
    sqlite3_finalize(chunkStmt);

    sqlite3_exec(db, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);

    const char* deleteSql = "DELETE FROM users WHERE user_id = ?";
    sqlite3_stmt* deleteStmt;
    sqlite3_prepare_v2(db, deleteSql, -1, &deleteStmt, nullptr);

    for (size_t userId : expiredUserIds) {
        sqlite3_bind_int64(deleteStmt, 1, userId);
        sqlite3_step(deleteStmt);
        sqlite3_reset(deleteStmt);
    }
    sqlite3_finalize(deleteStmt);

    return result;
}