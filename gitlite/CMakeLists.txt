cmake_minimum_required(VERSION 3.5)
project(Gitlite)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(${CMAKE_SOURCE_DIR}/include)
set(SOURCES
    main.cpp
    src/Utils.cpp
    src/GitliteException.cpp
    src/Commit.cpp
    src/Repository.cpp
)
add_executable(gitlite ${SOURCES})