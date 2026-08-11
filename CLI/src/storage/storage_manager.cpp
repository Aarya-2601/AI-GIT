#include "storage_manager.hpp"

#include <fstream>
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
            "File does not exist: " + filePath.string()
        );
    }

    // Store the actual bytes.
    // ObjectStore calculates SHA-256 and performs deduplication.
    std::string objectId =
        objectStore.store(filePath);

    // Get file size for metadata.
    long long fileSize =
        static_cast<long long>(
            std::filesystem::file_size(filePath)
        );

    // This is currently a normal whole file.
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
    return objectStore.retrieve(objectId);
}

bool StorageManager::objectExists(
    const std::string& objectId
) const
{
    return objectStore.exists(objectId);
}

}