cmake_minimum_required(VERSION 3.12...4.3.4)
project(AIGit LANGUAGES CXX)

#c++17 configuration
set(CMAKE_CXX_STANDARD 17)
#how strictly the compiler must adhere to the version standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#because vcpkg handles the downloading, these must be found locally.
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

#gather all source files into the variable sources
set(SOURCES
    "src/main.cpp"
    "src/core/hashing.cpp"
    "src/core/compression.cpp"
    "src/core/storage.cpp"
    "src/core/config.cpp"
    "src/core/index.cpp"
    "src/core/refs.cpp"

    "src/commands/hash_object.cpp"
    "src/commands/init.cpp"
    "src/commands/add.cpp"

    "src/models/blob.cpp"
    "src/models/tree.cpp"
    "src/models/commit.cpp"
)

add_executable(ai-git ${SOURCES})

#instead of global include_directories(src), we explicitly scope it to our binary object
target_include_directories(ai-git PRIVATE src)

#modern CMake targets auto pull all necessary downstream system dependencies
target_link_libraries(ai-git PRIVATE 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    ZLIB::ZLIB
)