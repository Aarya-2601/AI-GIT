#pragma once

#include "object_store.hpp"
#include "metadata_db.hpp"

#include <filesystem>
#include <string>

namespace Storage
{

class StorageManager
{
private:
    ObjectStore objectStore;
    MetadataDB metadataDB;

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

    bool objectExists(
        const std::string& objectId
    ) const;
};

}