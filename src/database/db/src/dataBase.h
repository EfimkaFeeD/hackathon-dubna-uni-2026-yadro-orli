#ifndef BD_BD_H
#define BD_BD_H

#include <sqlite3.h>
#include <iostream>
#include <string>
#include "enumsAndStructs.h"

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
};
#endif //BD_BD_H
