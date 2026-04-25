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