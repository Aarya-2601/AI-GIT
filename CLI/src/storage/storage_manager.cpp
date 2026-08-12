#include "storage_manager.hpp"

#include <filesystem>
#include <stdexcept>

namespace Storage
{

StorageManager::StorageManager(
    const std::filesystem::path& root
)
    : objectStore(root),
      metadataDB(root / "metadata.db")
{
}

void StorageManager::initialize()
{
    // Initialize both parts of the storage layer.

    objectStore.initialize();
    metadataDB.initialize();
}

std::string StorageManager::storeFile(
    const std::filesystem::path& filePath
)
{
    if (!std::filesystem::exists(filePath))
    {
        throw std::runtime_error(
            "File does not exist: " +
            filePath.string()
        );
    }

    // ------------------------------------------------
    // STEP 1:
    // Store the actual bytes.
    //
    // ObjectStore:
    //   file → SHA-256 → CAS object
    // ------------------------------------------------

    std::string objectId =
        objectStore.store(filePath);


    // ------------------------------------------------
    // STEP 2:
    // Record metadata about that object.
    //
    // MetadataDB:
    //   objectId + size + type → SQLite
    // ------------------------------------------------

    long long fileSize =
        static_cast<long long>(
            std::filesystem::file_size(filePath)
        );

    metadataDB.addObject(
        objectId,
        fileSize,
        "file"
    );

    return objectId;
}

std::string StorageManager::retrieveFile(
    const std::string& objectId
) const
{
    // StorageManager delegates actual byte retrieval
    // to ObjectStore.

    return objectStore.retrieve(objectId);
}

bool StorageManager::objectExists(
    const std::string& objectId
) const
{
    return objectStore.exists(objectId);
}

}