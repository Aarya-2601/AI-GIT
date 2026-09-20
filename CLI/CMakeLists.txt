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
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL REQUIRED)

set(SOURCES
    "src/main.cpp"

    "src/core/hashing.cpp"
    "src/core/compression.cpp"
    "src/core/config.cpp"
    "src/core/index.cpp"
    "src/core/chunking.cpp"
    "src/core/object_io.cpp"
    "src/core/filesystem.cpp"

    "src/commands/hash_object.cpp"
    "src/commands/init.cpp"
    "src/commands/add.cpp"
    "src/commands/status.cpp"
    "src/commands/commit.cpp"
    "src/commands/log.cpp"
    "src/commands/command_config.cpp"
    "src/commands/branch.cpp"
    "src/commands/checkout.cpp"
    "src/commands/push.cpp"
    "src/commands/pull.cpp"
    "src/commands/clone.cpp"

    "src/helpers/gitutils.cpp"
    "src/helpers/curl_helpers.cpp"

    "src/models/blob.cpp"
    "src/models/tree.cpp"
    "src/models/commit.cpp"

    "src/storage/object_store.cpp"
    "src/storage/metadata_db.cpp"
    "src/storage/storage_manager.cpp"

    # NEW: Remote CAS layer
    "src/RemoteCASClient/remote_cas_client.cpp"
    "src/RemoteCASClient/remote_sync_manager.cpp"
)

add_executable(ai-git ${SOURCES})

target_include_directories(ai-git PRIVATE
    src
)

target_link_libraries(ai-git PRIVATE
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    unofficial::sqlite3::sqlite3
    CURL::libcurl
)

add_executable(object-store-test
    src/storage/object_store_test.cpp
    src/storage/object_store.cpp
    src/core/hashing.cpp
    src/core/filesystem.cpp
    src/core/compression.cpp
)

target_include_directories(object-store-test PRIVATE
    src
)

target_link_libraries(object-store-test PRIVATE
    OpenSSL::Crypto
    ZLIB::ZLIB
)

add_executable(storage-manager-test
    src/storage/storage_manager_test.cpp
    src/storage/storage_manager.cpp
    src/storage/object_store.cpp
    src/storage/metadata_db.cpp
    src/core/hashing.cpp
    src/core/chunking.cpp
    src/core/filesystem.cpp
    src/core/compression.cpp
)

add_executable(remote-sync-test
    src/RemoteCASClient/remote_sync_test.cpp
    src/RemoteCASClient/remote_cas_client.cpp
    src/RemoteCASClient/remote_sync_manager.cpp

    src/storage/storage_manager.cpp
    src/storage/object_store.cpp
    src/storage/metadata_db.cpp

    src/core/hashing.cpp
    src/core/chunking.cpp
    src/core/filesystem.cpp
    src/core/compression.cpp
)

target_include_directories(remote-sync-test PRIVATE
    src
)

target_link_libraries(remote-sync-test PRIVATE
    OpenSSL::Crypto
    ZLIB::ZLIB
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
    CURL::libcurl
)

add_executable(push-metadata-test
    src/storage/push_metadata_test.cpp

    src/commands/init.cpp
    src/commands/add.cpp
    src/commands/commit.cpp

    src/core/hashing.cpp
    src/core/compression.cpp
    src/core/config.cpp
    src/core/index.cpp
    src/core/chunking.cpp
    src/core/object_io.cpp
    src/core/filesystem.cpp

    src/storage/object_store.cpp
    src/storage/metadata_db.cpp
    src/storage/storage_manager.cpp

    src/models/blob.cpp
    src/models/tree.cpp
    src/models/commit.cpp

    src/helpers/gitutils.cpp
)

target_include_directories(push-metadata-test PRIVATE
    src
)

target_link_libraries(push-metadata-test PRIVATE
    OpenSSL::Crypto
    ZLIB::ZLIB
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
)

add_executable(status-clean-test
    src/storage/status_clean_test.cpp

    src/commands/init.cpp
    src/commands/add.cpp
    src/commands/commit.cpp
    src/commands/status.cpp

    src/core/hashing.cpp
    src/core/compression.cpp
    src/core/config.cpp
    src/core/index.cpp
    src/core/chunking.cpp
    src/core/object_io.cpp
    src/core/filesystem.cpp

    src/storage/object_store.cpp
    src/storage/metadata_db.cpp
    src/storage/storage_manager.cpp

    src/models/blob.cpp
    src/models/tree.cpp
    src/models/commit.cpp

    src/helpers/gitutils.cpp
)

target_include_directories(status-clean-test PRIVATE
    src
)

target_link_libraries(status-clean-test PRIVATE
    OpenSSL::Crypto
    ZLIB::ZLIB
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
)

add_executable(checkout-index-sync-test
    src/storage/checkout_index_sync_test.cpp

    src/commands/init.cpp
    src/commands/add.cpp
    src/commands/commit.cpp
    src/commands/branch.cpp
    src/commands/checkout.cpp

    src/core/hashing.cpp
    src/core/compression.cpp
    src/core/config.cpp
    src/core/index.cpp
    src/core/chunking.cpp
    src/core/object_io.cpp
    src/core/filesystem.cpp

    src/storage/object_store.cpp
    src/storage/metadata_db.cpp
    src/storage/storage_manager.cpp

    src/models/blob.cpp
    src/models/tree.cpp
    src/models/commit.cpp

    src/helpers/gitutils.cpp
)

target_include_directories(checkout-index-sync-test PRIVATE
    src
)

target_link_libraries(checkout-index-sync-test PRIVATE
    OpenSSL::Crypto
    ZLIB::ZLIB
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
)

target_include_directories(storage-manager-test PRIVATE
    src
)

target_link_libraries(storage-manager-test PRIVATE
    OpenSSL::Crypto
    ZLIB::ZLIB
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
)

add_executable(hash-object-parity-test
    src/storage/hash_object_parity_test.cpp

    src/commands/init.cpp
    src/commands/add.cpp
    src/commands/hash_object.cpp

    src/core/hashing.cpp
    src/core/compression.cpp
    src/core/config.cpp
    src/core/index.cpp
    src/core/chunking.cpp
    src/core/object_io.cpp
    src/core/filesystem.cpp

    src/storage/object_store.cpp
    src/storage/metadata_db.cpp
    src/storage/storage_manager.cpp

    src/models/blob.cpp
    src/models/tree.cpp
    src/models/commit.cpp

    src/helpers/gitutils.cpp
)

target_include_directories(hash-object-parity-test PRIVATE
    src
)

target_link_libraries(hash-object-parity-test PRIVATE
    OpenSSL::Crypto
    ZLIB::ZLIB
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
)

install(
    TARGETS ai-git
    DESTINATION bin
)

set(CPACK_PACKAGE_NAME "AI-Git")
set(CPACK_PACKAGE_VENDOR "The AI-Git Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-Performance Standalone VCS for AI & ML Models")
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