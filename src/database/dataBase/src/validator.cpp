#include "validator.h"

void validator::addError(const std::string& table_name, const std::string& column_name, const std::string& message) {
    errors.push_back("[" + table_name + "][" + column_name + "] " + message);
}

void validator::addError(const std::string& table_name, const std::string& message) {
    errors.push_back("[" + table_name + "] " + message);
}

bool validator::isValidIdentifier(const std::string& name) {
    if (name.empty()) return false;
    std::regex identifier_regex("^[a-zA-Z_][a-zA-Z0-9_]*$");
    return std::regex_match(name, identifier_regex);
}

bool validator::validateSchema(const std::vector<table>& tables) {
    errors.clear();

    if (tables.empty()) {
        errors.push_back("The schema does not contain any tables.");
        return false;
    }

    std::set<std::string> tableNames;

    for (const auto& tbl : tables) {
        validateTable(tbl, tableNames);
    }

    if (errors.empty()) {
        std::cout << "OK" << std::endl;
        return true;
    } else {
        std::cout << "Errors: " << errors.size() << std::endl;
        for (const auto& err : errors) {
            std::cout << "   " << err << std::endl;
        }
        return false;
    }
}

void validator::validateTable(const table& table, std::set<std::string>& tableNames) {
    if (!isValidIdentifier(table.name)) {
        addError(table.name, "The table name contains invalid characters");
        return;
    }

    if (tableNames.count(table.name)) {
        addError(table.name, "A table with this name already exists");
        return;
    }
    tableNames.insert(table.name);

    if (table.columns.empty()) {
        addError(table.name, "The table does not contain columns");
        return;
    }

    std::set<std::string> columnNames;
    bool hasPrimaryKey = false;

    for (const auto& col : table.columns) {
        validateColumn(table.name, col, columnNames, hasPrimaryKey);
    }

    if (!hasPrimaryKey) {
        addError(table.name, "The table must contain a PRIMARY KEY");
    }
}

void validator::validateColumn(const std::string& tabName, const column& col,
                    std::set<std::string>& columnNames, bool& hasPrimaryKey) {
    if (!isValidIdentifier(col.name)) {
        addError(tabName, col.name, "The column name contains invalid characters");
        std::cout << "The column name contains invalid characters" << std::endl;
        return;
    }

    if (columnNames.count(col.name)) {
        addError(tabName, col.name,
                 "A column with this name already exists in this table");
        std::cout << "A column with this name already exists in this table" << std::endl;
        return;
    }
    columnNames.insert(col.name);

    if (col.cons == constraint::PRIMARY_KEY ||
        col.cons == constraint::PRIMARY_KEY_AUTOINCREMENT) {

        if (hasPrimaryKey) {
            addError(tabName, col.name, "There can only be one PRIMARY KEY in a table");
            std::cout << "There can only be one PRIMARY KEY in a table" << std::endl;
            return;
        }
        hasPrimaryKey = true;

        if (col.cons == constraint::PRIMARY_KEY_AUTOINCREMENT &&
            col.type != dbTypes::INTEGER) {
            addError(tabName, col.name,
                     "AUTOINCREMENT can only be used with the INTEGER type");
            std::cout << "AUTOINCREMENT can only be used with the INTEGER type" << std::endl;
            return;
        }
    }

    if (col.minValue.has_value() || col.maxValue.has_value()) {
        if (col.type != dbTypes::INTEGER && col.type != dbTypes::REAL && col.type != dbTypes::NUMERIC) {
            addError(tabName, col.name,
                     "minValue/maxValue can only be used with numeric types.");
            std::cout << "minValue/maxValue can only be used with numeric types." << std::endl;
            return;
        }

        if (col.minValue.has_value() && col.maxValue.has_value()) {
            if (col.minValue.value() > col.maxValue.value()) {
                addError(tabName, col.name, "minValue cannot be greater than maxValue");
                std::cout << "minValue cannot be greater than maxValue" << std::endl;
                return;
            }
        }
    }

    if (col.minLength.has_value() || col.maxLength.has_value()) {
        if (col.type != dbTypes::TEXT) {
            addError(tabName, col.name,
                     "minLength/maxLength can only be used with the TEXT type.");
            return;
        }

        if (col.minLength.has_value() && col.maxLength.has_value()) {
            if (col.minLength.value() > col.maxLength.value()) {
                addError(tabName, col.name, "minLength cannot be greater than maxLength");
                return;
            }
        }
    }

    if (!col.allowedValues.empty()) {
        std::set<std::string> uniqueValues(col.allowedValues.begin(), col.allowedValues.end());
        if (uniqueValues.size() != col.allowedValues.size()) {
            addError(tabName, col.name, "allowedValues contains duplicates");
            return;
        }
    }

    if (col.cons == constraint::FOREIGN_KEY) {
        if (col.foreignTable.empty()) {
            addError(tabName, col.name, "FOREIGN KEY specified, but no table specified");
            return;
        }
        if (col.foreignColumn.empty()) {
            addError(tabName, col.name, "FOREIGN KEY specified, but no column specified");
        }
    }
}

