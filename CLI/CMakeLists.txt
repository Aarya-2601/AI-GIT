cmake_minimum_required(VERSION 3.12...4.3.4)

project(AIGit LANGUAGES CXX)

# C++17 configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MSVC_RUNTIME_LIBRARY
    "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

# Dependencies
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SQLite3 REQUIRED)


# All source files
set(SOURCES

    # Main
    "src/main.cpp"

    # Core
    "src/core/hashing.cpp"
    "src/core/compression.cpp"
    "src/core/storage.cpp"
    "src/core/config.cpp"
    "src/core/index.cpp"
    "src/core/refs.cpp"

    # Commands
    "src/commands/hash_object.cpp"
    "src/commands/init.cpp"
    "src/commands/add.cpp"
    "src/commands/status.cpp"
    "src/commands/commit.cpp"
    "src/commands/log.cpp"
    "src/commands/command_config.cpp"
    "src/commands/branch.cpp"

    # Helpers
    "src/helpers/gitutils.cpp"

    # Models
    "src/models/blob.cpp"
    "src/models/tree.cpp"
    "src/models/commit.cpp"

    # Storage
    "src/storage/object_store.cpp"
    "src/storage/metadata_db.cpp"
)


# Main executable
add_executable(ai-git ${SOURCES})


# Include project headers
target_include_directories(ai-git PRIVATE
    src
)


# Link dependencies
target_link_libraries(ai-git PRIVATE

    OpenSSL::SSL
    OpenSSL::Crypto

    ZLIB::ZLIB

    SQLite::SQLite3
)


# Installation
install(
    TARGETS ai-git
    DESTINATION bin
)


# CPACK INSTALLATION AND PACKAGING CONFIGURATION

set(CPACK_PACKAGE_NAME "AI-Git")

set(CPACK_PACKAGE_VENDOR
    "The Ai-Git Team"
)

set(
    CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "High-Performance Standalone VCS for AI & ML Models"
)

set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")


# Windows setup wizard
set(CPACK_NSIS_MODIFY_PATH ON)


if(WIN32)

    set(CPACK_GENERATOR "NSIS;ZIP")

elseif(APPLE)

    set(CPACK_GENERATOR "DragNDrop;TGZ")

else()

    set(CPACK_GENERATOR "DEB;RPM;TGZ")

endif()


include(CPack)