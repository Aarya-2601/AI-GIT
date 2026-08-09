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
    "src/commands/status.cpp"
    "src/commands/commit.cpp"
    "src/commands/log.cpp"
    "src/commands/command_config.cpp"
    "src/commands/branch.cpp"

    "src/models/blob.cpp"
    "src/models/tree.cpp"
    "src/models/commit.cpp"

    "src/helpers/gitutils.cpp"
    "src/storage/object_store.cpp"

)

add_executable(ai-git ${SOURCES})

add_executable(object-store-test
    "src/storage/object_store_test.cpp"
    "src/storage/object_store.cpp"
    "src/core/hashing.cpp"
)

target_include_directories(object-store-test PRIVATE src)

target_link_libraries(object-store-test PRIVATE
    OpenSSL::Crypto
)

#instead of global include_directories(src), we explicitly scope it to our binary object
target_include_directories(ai-git PRIVATE src)

#modern CMake targets auto pull all necessary downstream system dependencies
target_link_libraries(ai-git PRIVATE 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    ZLIB::ZLIB
)

#CPACK INSTALLATION AND PACKAGING CONFIGURATION

#put executable in the bin folder
install(TARGETS ai-git DESTINATION bin)

#package metadata info
set(CPACK_PACKAGE_NAME "AI-Git")
set(CPACK_PACKAGE_VENDOR "The Ai-Git Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-Performance Standalone VCS for AI & ML Models")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")

#for Windows setup wizards (NSIS), this automatically adds a checkbox asking the user if they want to append ai-git to system environment variables so they can run ai-git from any terminal prompt instantly.
set(CPACK_NSIS_MODIFY_PATH ON)

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
else()
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
endif()

include(CPack)