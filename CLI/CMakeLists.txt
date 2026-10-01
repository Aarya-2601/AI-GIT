cmake_minimum_required(VERSION 3.12...4.3.4)

project(AIGit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MSVC_RUNTIME_LIBRARY
    "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)

set(SOURCES

    #main
    "src/main.cpp"

    #core
    "src/core/storage.cpp"
    "src/core/config.cpp"
    "src/core/index.cpp"
    "src/core/refs.cpp"

    #modular task folders
    "src/hashing/hashing.cpp"
    "src/compression/compression.cpp"
    "src/inspection/file_inspector.cpp"
    "src/chunking/asymmetric_chunker.cpp"

    #commands
    "src/commands/hash_object.cpp"
    "src/commands/init.cpp"
    "src/commands/add.cpp"
    "src/commands/status.cpp"
    "src/commands/commit.cpp"
    "src/commands/log.cpp"
    "src/commands/command_config.cpp"
    "src/commands/branch.cpp"

    #helpers
    "src/helpers/gitutils.cpp"

    #models
    "src/models/blob.cpp"
    "src/models/tree.cpp"
    "src/models/commit.cpp"

    #storage
    "src/storage/object_store.cpp"
    "src/storage/metadata_db.cpp"
    "src/storage/storage_manager.cpp"
)


add_executable(ai-git ${SOURCES})

target_include_directories(ai-git PRIVATE
    src
    src/inspection
    src/chunking
    src/compression
    src/hashing
)

target_link_libraries(ai-git PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    SQLite::SQLite3
    CURL::libcurl
)

add_executable(object-store-test
    src/storage/object_store_test.cpp
    src/storage/object_store.cpp
    src/hashing/hashing.cpp
)

target_include_directories(object-store-test PRIVATE
    src
    src/hashing
)

target_link_libraries(object-store-test PRIVATE
    OpenSSL::Crypto
)

# ------------------------------------------------------------
# Format Inspector Verification Tool (Task 1.1)
# ------------------------------------------------------------

add_executable(aigit-inspect
    src/tools/inspect_main.cpp
    src/inspection/file_inspector.cpp
)

target_include_directories(aigit-inspect PRIVATE
    src
    src/inspection
)

# ------------------------------------------------------------
# Asymmetric Chunker Verification Tool (Task 1.2)
# ------------------------------------------------------------

add_executable(aigit-chunk
    src/tools/chunker_test_main.cpp
    src/chunking/asymmetric_chunker.cpp
    src/inspection/file_inspector.cpp
    src/hashing/hashing.cpp
)

target_include_directories(aigit-chunk PRIVATE
    src
    src/inspection
    src/chunking
    src/hashing
)

target_link_libraries(aigit-chunk PRIVATE
    OpenSSL::Crypto
)

# ------------------------------------------------------------
# Polymorphic Codec Verification Tool (Task 1.3)
# ------------------------------------------------------------

add_executable(aigit-codec
    src/tools/codec_test_main.cpp
    src/compression/polymorphic_codec.cpp
    src/compression/compression.cpp
    src/chunking/asymmetric_chunker.cpp
    src/inspection/file_inspector.cpp
    src/hashing/hashing.cpp
)

target_include_directories(aigit-codec PRIVATE
    src
    src/inspection
    src/chunking
    src/compression
    src/hashing
)

target_link_libraries(aigit-codec PRIVATE
    OpenSSL::Crypto
    ZLIB::ZLIB
)

# ------------------------------------------------------------
# Semantic Structural Differ & Metadata Harvester (Task 1.4)
# ------------------------------------------------------------

add_executable(aigit-diff
    src/tools/diff_main.cpp
    src/diffing/structural_differ.cpp
    src/diffing/metadata_harvester.cpp
    src/inspection/file_inspector.cpp
    src/hashing/hashing.cpp
)

target_include_directories(aigit-diff PRIVATE
    src
    src/diffing
    src/inspection
    src/hashing
)

target_link_libraries(aigit-diff PRIVATE
    OpenSSL::Crypto
    SQLite::SQLite3
)

# ------------------------------------------------------------
# Global CAS Sync & Remote Delta Engine (Task 1.5)
# ------------------------------------------------------------

add_executable(aigit-sync
    src/tools/sync_main.cpp
    src/sync/cas_sync_engine.cpp
    src/compression/polymorphic_codec.cpp
    src/compression/compression.cpp
    src/chunking/asymmetric_chunker.cpp
    src/inspection/file_inspector.cpp
    src/hashing/hashing.cpp
)

target_include_directories(aigit-sync PRIVATE
    src
    src/sync
    src/compression
    src/chunking
    src/inspection
    src/hashing
)

target_compile_definitions(aigit-sync PRIVATE
    CURL_STATICLIB
)

target_link_libraries(aigit-sync PRIVATE
    OpenSSL::Crypto
    ZLIB::ZLIB
    CURL::libcurl
    ws2_32
    crypt32
    advapi32
    user32
    wldap32
    normaliz
    bcrypt
    secur32
    iphlpapi
)

install(
    TARGETS ai-git aigit-inspect aigit-chunk aigit-codec aigit-diff aigit-sync
    DESTINATION bin
)


set(CPACK_PACKAGE_NAME "AI-Git")

set(CPACK_PACKAGE_VENDOR
    "The AI-Git Team"
)

set(
    CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "High-Performance Standalone VCS for AI & ML Models"
)

set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")

set(CPACK_NSIS_MODIFY_PATH ON)

if(WIN32)

    set(CPACK_GENERATOR "NSIS;ZIP")

elseif(APPLE)

    set(CPACK_GENERATOR "DragNDrop;TGZ")

else()

    set(CPACK_GENERATOR "DEB;RPM;TGZ")

endif()

include(CPack)