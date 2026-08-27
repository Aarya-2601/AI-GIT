#pragma once

#include "object_store.hpp"
#include "metadata_db.hpp"
#include "../core/chunking.hpp"

#include <filesystem>
#include <string>

namespace Storage
{

class StorageManager
{
private:
    ObjectStore objectStore;
    MetadataDB metadataDB;

    std::string readSlice(
        const std::filesystem::path& filePath,
        size_t offset,
        size_t length
    ) const;

public:
    explicit StorageManager(
        const std::filesystem::path& root
    );

    void initialize();

    std::string storeFile(
        const std::filesystem::path& filePath
    );

    std::string retrieveFile(
        const std::string& objectId
    ) const;

    void restoreFile(
        const std::string& objectId,
        const std::filesystem::path& destinationPath
    ) const;

    bool objectExists(
        const std::string& objectId
    ) const;
};

}