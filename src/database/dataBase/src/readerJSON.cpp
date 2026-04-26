#include "readerJSON.h"

template<typename T>
std::optional<T> getOptional(const nlohmann::json& j, const std::string& key) {
    auto it = j.find(key);
    if (it != j.end() && !it->is_null()) {
        return it->get<T>();
    }
    return std::nullopt;
}


void readerJSON::read(const std::string &fileNameOrPath) {
    std::ifstream file(fileNameOrPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        return;
    }
    nlohmann::json data;
    try {
        file >> data;
    } catch (const nlohmann::json::parse_error& err) {
        std::cerr << "JSON parsing error: " << err.what() << std::endl;
        return;
    }
    file.close();
    auto tablesIntern = data["tables"];
    for (const auto& tblJSON : tablesIntern) {
        table tbl;
        tbl.name = tblJSON["name"];
        auto columns = tblJSON["columns"];
        for (const auto& colJSON : columns) {
            column col;
            col.name = colJSON["name"];
            col.type = stringToType(colJSON["type"]);
            col.readyDefaultValue = stringToDefaultValues(colJSON["readyDefaultValue"]);
            col.customDefaultValue = colJSON["customDefaultValue"];
            col.cons = stringToConstraint(colJSON["constraint"]);
            col.foreignTable = colJSON["foreignTable"];
            col.foreignColumn = colJSON["foreignColumn"];
            col.minValue = getOptional<int>(colJSON, "minValue");
            col.maxValue = getOptional<int>(colJSON, "maxValue");
            col.minLength = getOptional<int>(colJSON, "minLength");
            col.maxLength = getOptional<int>(colJSON, "maxLength");
            if (colJSON.contains("allowedValues")) {
                const auto& allowedJson = colJSON["allowedValues"];
                if (allowedJson.is_array()) {
                    for (const auto& val : allowedJson) {
                        if (val.is_string()) {
                            col.allowedValues.emplace_back(val.get<std::string>());
                        } else if (val.is_number()) {
                            col.allowedValues.emplace_back(std::to_string(val.get<int>()));
                        } else if (val.is_null()) {
                            col.allowedValues.emplace_back("null");
                        }
                    }
                }
            }
            tbl.columns.push_back(col);
        }
        tables.push_back(tbl);
    }
}

std::vector<table> readerJSON::getTables() {
    return tables;
}