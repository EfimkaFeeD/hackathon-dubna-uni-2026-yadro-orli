cmake_minimum_required(VERSION 3.28)
project(bd)

set(CMAKE_CXX_STANDARD 17)
file(GLOB_RECURSE src "src/*.cpp")
list(APPEND src "main.cpp")
find_package(SQLite3 REQUIRED)
find_package(nlohmann_json REQUIRED)
add_executable(bd ${src})
add_custom_command(
        TARGET bd
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/src/confCreateDB.json
        ${CMAKE_CURRENT_BINARY_DIR}/confCreateDB.json
        COMMENT "Copying config file to build directory"
)
target_link_libraries(bd
        PRIVATE
        SQLite::SQLite3
        nlohmann_json::nlohmann_json
)