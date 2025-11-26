cmake_minimum_required(VERSION 3.5)
project(Gitlite)

# 建议指定 C++ 标准，防止编译特性不兼容
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. 添加头文件搜索路径
include_directories(${CMAKE_SOURCE_DIR}/include)

# 2. 设置源文件列表
# 注意：这里假设 CMakeLists.txt 在根目录
set(SOURCES
    main.cpp
    src/Utils.cpp
    src/GitliteException.cpp
    src/Commit.cpp
    src/Repository.cpp
    # 如果你添加了其他类，记得在这里加上对应的 .cpp 文件
)

# 3. 生成可执行文件，名称必须是 gitlite
add_executable(gitlite ${SOURCES})