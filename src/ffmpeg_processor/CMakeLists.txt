# Системный ffmpeg >= 5.1
# Зависимости: libfvad, libswresample, libavfilter, libavutil, libavcodec, libavformat

cmake_minimum_required(VERSION 3.31)

project(podcast_silence_remover C CXX)

set(CMAKE_CXX_STANDARD 20)

find_library(FVAD_LIB NAMES fvad REQUIRED)
find_library(AVFORMAT_LIB NAMES avformat REQUIRED)
find_library(AVCODEC_LIB NAMES avcodec REQUIRED)
find_library(AVFILTER_LIB NAMES avfilter REQUIRED)
find_library(SWRESAMPLE_LIB NAMES swresample REQUIRED)
find_library(AVUTIL_LIB NAMES avutil REQUIRED)

find_path(AV_INCLUDE_DIR NAMES libavcodec/avcodec.h REQUIRED)
find_path(FVAD_INCLUDE_DIR NAMES fvad.h REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE MY_SOURCES "src/*.cpp" "src/*.hpp" "src/*.c" "src/*.h")

add_executable(${PROJECT_NAME} ${MY_SOURCES})

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

