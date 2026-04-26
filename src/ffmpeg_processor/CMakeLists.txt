# Системный ffmpeg >= 5.1
# Зависимости: libfvad, libswresample, libavfilter, libavutil, libavcodec, libavformat
# -DBUILD_SHARED_LIB=ON для сборки динамической библиотеки

cmake_minimum_required(VERSION 3.31)

project(ffmpeg_processor C CXX)

set(CMAKE_CXX_STANDARD 20)

option(BUILD_SHARED_LIB "Build as shared library instead of executable" OFF)

find_library(FVAD_LIB NAMES fvad REQUIRED)
find_library(AVFORMAT_LIB NAMES avformat REQUIRED)
find_library(AVCODEC_LIB NAMES avcodec REQUIRED)
find_library(AVFILTER_LIB NAMES avfilter REQUIRED)
find_library(SWRESAMPLE_LIB NAMES swresample REQUIRED)
find_library(AVUTIL_LIB NAMES avutil REQUIRED)

find_path(AV_INCLUDE_DIR NAMES libavcodec/avcodec.h REQUIRED)
find_path(FVAD_INCLUDE_DIR NAMES fvad.h REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE LIB_SOURCES "src/*.cpp" "src/*.hpp" "src/*.c" "src/*.h")

list(FILTER LIB_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

set(MAIN_SOURCES "src/main.cpp")

if (BUILD_SHARED_LIB)
    add_library(${PROJECT_NAME} SHARED ${LIB_SOURCES})

    option(BUILD_TEST_EXECUTABLE "Build test executable linking to the shared library" ON)
    if (BUILD_TEST_EXECUTABLE)
        add_executable(silence_test ${MAIN_SOURCES})
        target_link_libraries(silence_test PRIVATE ${PROJECT_NAME})
    endif ()
else ()
    add_executable(${PROJECT_NAME} ${LIB_SOURCES} ${MAIN_SOURCES})
endif ()

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${AV_INCLUDE_DIR}
        ${FVAD_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ${FVAD_LIB}
        ${AVFORMAT_LIB}
        ${AVCODEC_LIB}
        ${AVFILTER_LIB}
        ${SWRESAMPLE_LIB}
        ${AVUTIL_LIB}
)


if (BUILD_SHARED_LIB)
    target_include_directories(${PROJECT_NAME} PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${AV_INCLUDE_DIR}
            ${FVAD_INCLUDE_DIR}
    )
    install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
            DESTINATION include/${PROJECT_NAME}
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
    )
endif ()
