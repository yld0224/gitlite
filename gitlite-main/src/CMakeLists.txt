cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Commit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GitliteException.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Repository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp
)

add_executable(code ${SOURCES})