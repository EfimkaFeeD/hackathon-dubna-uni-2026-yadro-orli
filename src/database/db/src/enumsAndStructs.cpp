#include "enumsAndStructs.h"

std::string typeToString(dbTypes type) {
    switch (type) {
        case dbTypes::INTEGER: return "INTEGER";
        case dbTypes::REAL: return "REAL";
        case dbTypes::BLOB: return "BLOB";
        case dbTypes::TEXT: return "TEXT";
        case dbTypes::NUMERIC: return "NUMERIC";
        default: return "TEXT";
    }
}

dbTypes stringToType(const std::string& type) {
    if (type == "INTEGER") return dbTypes::INTEGER;
    if (type == "REAL") return dbTypes::REAL;
    if (type == "BLOB") return dbTypes::BLOB;
    if (type == "TEXT") return dbTypes::TEXT;
    if (type == "NUMERIC") return dbTypes::NUMERIC;
    return dbTypes::TEXT;
}

std::string constraintToString(constraint cons) {
    switch (cons) {
        case constraint::NONE: return "NONE";
        case constraint::PRIMARY_KEY: return "PRIMARY KEY";
        case constraint::PRIMARY_KEY_AUTOINCREMENT: return "PRIMARY KEY AUTOINCREMENT";
        case constraint::NOT_NULL: return "NOT NULL";
        case constraint::UNIQUE: return "UNIQUE";
        case constraint::AUTOINCREMENT: return "AUTOINCREMENT";
        case constraint::FOREIGN_KEY: return "REFERENCE ";
        default: return "NONE";
    }
}

constraint stringToConstraint(const std::string& cons) {
    if (cons == "NONE") return constraint::NONE;
    if (cons == "PRIMARY KEY") return constraint::PRIMARY_KEY;
    if (cons == "PRIMARY KEY AUTOINCREMENT") return constraint::PRIMARY_KEY_AUTOINCREMENT;
    if (cons == "NOT NULL") return constraint::NOT_NULL;
    if (cons == "UNIQUE") return constraint::UNIQUE;
    if (cons == "AUTOINCREMENT") return constraint::AUTOINCREMENT;
    if (cons == "REFERENCE ") return constraint::FOREIGN_KEY;
    return constraint::NONE;
}

std::string defaultValuesToString(defaultValues defVal) {
    switch (defVal) {
        case defaultValues::NONE: return "NONE";
        case defaultValues::CURRENT_TIMESTAMP: return "CURRENT TIMESTAMP";
        case defaultValues::CURRENT_TIME: return "CURRENT TIME";
        case defaultValues::CURRENT_DATE: return "CURRENT DATE";
        case defaultValues::ZERO: return "0";
        case defaultValues::EMPTY_STRING: return "";
        case defaultValues::CUSTOM_DEFAULT_VALUE: return "CUSTOM DEFAULT VALUE";
        default: return "NONE";
    }
}

defaultValues stringToDefaultValues(const std::string& defVal) {
    if (defVal == "NONE") return defaultValues::NONE;
    if (defVal == "CURRENT TIMESTAMP") return defaultValues::CURRENT_TIMESTAMP;
    if (defVal == "CURRENT TIME") return defaultValues::CURRENT_TIME;
    if (defVal == "CURRENT DATE") return defaultValues::CURRENT_DATE;
    if (defVal == "0") return defaultValues::ZERO;
    if (defVal == "") return defaultValues::EMPTY_STRING;
    if (defVal == "CUSTOM DEFAULT VALUE") return defaultValues::CUSTOM_DEFAULT_VALUE;
    return defaultValues::NONE;
}