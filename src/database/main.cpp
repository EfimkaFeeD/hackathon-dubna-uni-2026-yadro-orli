#include <sqlite3.h>
#include "./src/validator.h"
#include "./src/dataBase.h"
#include "./src/readerJSON.h"

int main() {
    readerJSON reader;
    reader.read("C:\\Users\\Savel\\CLionProjects\\dataBase\\src\\confCreateDB.json");
    std::vector<table> tables = reader.getTables();
    validator val;
    if (val.validateSchema(tables)) {
        dataBase db;
        db.createAllTables(tables);
    } else {
        std::cout << "not validate schema" << std::endl;
    }
}
