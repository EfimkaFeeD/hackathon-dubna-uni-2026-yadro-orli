#ifndef BD_READERJSON_H
#define BD_READERJSON_H

#include <nlohmann/json.hpp>
#include <fstream>
#include "enums_and_structs.h"
#include <iostream>

class readerJSON {
    std::vector<table> tables;
public:
    void read(const std::string& fileNameOrPath);
    std::vector<table> getTables();
};

#endif //BD_READERJSON_H
