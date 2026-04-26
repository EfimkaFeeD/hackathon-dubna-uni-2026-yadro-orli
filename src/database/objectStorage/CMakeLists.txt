cmake_minimum_required(VERSION 3.28)
project(hacaton)

set(CMAKE_CXX_STANDARD 17)
set(VCPKG_ROOT "C:/vcpkg")

file(GLOB_RECURSE hakaton "src/*cpp")
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/libs
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${VCPKG_ROOT}/installed/x64-windows/include
)
link_directories(${VCPKG_ROOT}/installed/x64-windows/lib)
list(APPEND hakaton main.cpp)

add_executable(hacaton ${hakaton})

target_link_libraries(hacaton ws2_32)