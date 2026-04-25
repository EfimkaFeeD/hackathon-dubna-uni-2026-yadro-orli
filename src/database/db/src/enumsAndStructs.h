#ifndef BD_ENUMS_H
#define BD_ENUMS_H

#include <string>
#include <vector>
#include <optional>

enum class dbTypes {
    INTEGER,
    REAL,
    TEXT,
    BLOB,
    NUMERIC
};

enum class constraint {
    NONE,
    PRIMARY_KEY,
    PRIMARY_KEY_AUTOINCREMENT,
    NOT_NULL,
    UNIQUE,
    AUTOINCREMENT,
    FOREIGN_KEY
};

enum class defaultValues {
    NONE,
    CURRENT_TIMESTAMP,
    CURRENT_DATE,
    CURRENT_TIME,
    ZERO,
    EMPTY_STRING,
    CUSTOM_DEFAULT_VALUE
};

struct column {
    std::string name;
    dbTypes type;
    defaultValues readyDefaultValue = defaultValues::NONE;
    std::string customDefaultValue = "";
    constraint cons = constraint::NONE;
    std::string foreignTable;
    std::string foreignColumn;

    std::optional<int> minValue;
    std::optional<int> maxValue;
    std::optional<int> minLength;
    std::optional<int> maxLength;
    std::vector<std::string> allowedValues;
};

struct table {
    std::string name;
    std::vector<column> columns;
};

std::string typeToString(dbTypes type);
dbTypes stringToType(const std::string& type);
std::string constraintToString(constraint cons);
constraint stringToConstraint(const std::string& cons);
std::string defaultValuesToString(defaultValues defVal);
defaultValues stringToDefaultValues(const std::string& defVal);

#endif //BD_ENUMS_H
