cmake_minimum_required(VERSION 3.12...4.3.4)
project(AIGit LANGUAGES CXX)

#c++17 configuration
set(CMAKE_CXX_STANDARD 17)
#how strictly the compiler must adhere to the version standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LINK_LIBRARIES "")

#look for OpenSSL in hosting OS
find_package(OpenSSL)

#automation of OpenSSL download
if(NOT OpenSSL_FOUND)
    message(STATUS "Local OpenSSL not found. Downloading from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        openssl
        GIT_REPOSITORY https://github.com/openssl/openssl.git
        GIT_TAG        openssl-3.4.0
    )
    # Download and make the OpenSSL targets available to our build system
    FetchContent_MakeAvailable(openssl)
else()
    message(STATUS "Native OpenSSL detection complete.")
    list(APPEND LINK_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
endif()

find_package(zlib)

if(NOT ZLIB_FOUND)
    message(STATUS "Local zlib tracking target missing. Fetching source from GitHub.")
    include(FetchContent)
    FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/facebook/zstd.git
        GIT_TAG        zstd 1.5.7 -1
    )
    FetchContent_MakeAvailable(zlib)
    add_library(ZlibFallback ALIAS zlibstatic)
    list(APPEND LINK_LIBRARIES ZlibFallback)
else()
    message(STATUS "Native Zlib detection complete.")
    list(APPEND LINK_LIBRARIES ZLIB::ZLIB)

endif()


#gather all source files into the variable sources
set(SOURCES
    "src/main.cpp"
    "src/core/hashing.cpp"
    "src/core/compression.cpp"
    "src/core/storage.cpp"
    "src/commands/hash_object.cpp"
    "src/commands/init.cpp"
    "src/commands/add.cpp"

    # to be added when we write the code
    # "src/models/blob.cpp"
    # "src/models/tree.cpp"
    # "src/models/commit.cpp"
)

add_executable(ai-git ${SOURCES})

#instead of global include_directories(src), we explicitly scope it to our binary object
target_include_directories(ai-git PRIVATE src)

#link OpenSSl binary to the backend
if(NOT OpenSSL_FOUND AND NOT TARGET openssl)
    target_link_libraries(ai-git PRIVATE ssl crypto)
endif()

if(NOT ZLIB_FOUND AND NOT TARGET zibstatic)
    target_link_libraries(ai-git PRIVATE z)
endif()