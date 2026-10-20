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

    // Computes the object ID `storeFile` would return for `filePath`'s
    // current on-disk content, without writing anything. Uses the exact
    // same raw-bytes / chunk-manifest hashing storeFile does, so its
    // result is directly comparable against an index entry's hash to
    // detect whether a tracked file has actually changed.
    std::string computeObjectId(
        const std::filesystem::path& filePath
    ) const;

    void storeObject(
        const std::string& objectId,
        const std::string& data,
        const std::string& type
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