cmake_minimum_required(VERSION 3.12...4.3.4
                        DESCRIPTION "AIGit project"
                        LANGUAGES CXX)
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

#header files location
include_directories(src)
#OPENSSL_INCLUDE_DIR is a dynamic variable that is created by CMake when it is hunting for this lib
include_directories(${OPENSSL_INCLUDE_DIR})

#gather all source files into the variable sources
file(GLOB_RECURSE SOURCES 
    "src/main.cpp"
    "src/commands/*.cpp"
    "src/core/*.cpp"
)

#final output name of the executable
add_executable(ai-git ${SOURCES})

#link OpenSSl binary to the backend
if(OpenSSL_FOUND)
    target_link_libraries(ai-git PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
    target_link_libraries(ai-git PRIVATE ssl crypto)
endif()