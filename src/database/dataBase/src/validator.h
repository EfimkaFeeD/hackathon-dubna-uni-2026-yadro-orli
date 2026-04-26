#ifndef BD_VALIDATOR_H
#define BD_VALIDATOR_H

#include "enums_and_structs.h"
#include <string>
#include <vector>
#include <regex>
#include <set>
#include <iostream>

class validator {
private:
    std::vector<std::string> errors;

    void addError(const std::string& table_name, const std::string& column_name, const std::string& message);

    void addError(const std::string& table_name, const std::string& message);

    bool isValidIdentifier(const std::string& name);
public:
    bool validateSchema(const std::vector<table>& tables);

private:
    void validateTable(const table& table, std::set<std::string>& tableNames);

    void validateColumn(const std::string& tabName, const column& col,
                        std::set<std::string>& columnNames, bool& hasPrimaryKey);
};

#endif //BD_VALIDATOR_H
