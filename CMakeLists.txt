cmake_minimum_required(VERSION 3.12...4.3.4)
project(AIGit LANGUAGES CXX)

#c++17 configuration
set(CMAKE_CXX_STANDARD 17)
#how strictly the compiler must adhere to the version standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
endif()

#gather all source files into the variable sources
set(SOURCES
    "src/main.cpp"
    "src/core/hashing.cpp"
    "src/commands/hash_object.cpp"
    "src/commands/init.cpp"
)

add_executable(ai-git ${SOURCES})

#instead of global include_directories(src), we explicitly scope it to our binary object
target_include_directories(ai-git PRIVATE src)

#link OpenSSl binary to the backend
if(OpenSSL_FOUND)
    target_link_libraries(ai-git PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
    target_link_libraries(ai-git PRIVATE ssl crypto)
endif()