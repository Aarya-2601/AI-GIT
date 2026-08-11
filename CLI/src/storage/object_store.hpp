#pragma once

#include <filesystem>
#include <string>

#include "metadata_db.hpp"

namespace Storage {

class ObjectStore {
private:
    std::filesystem::path rootPath;
    MetadataDB metadataDB;

public:
    explicit ObjectStore(const std::filesystem::path& root);

    void initialize();

    bool exists(const std::string& objectId) const;

    std::string store(const std::filesystem::path& filePath);

    std::string retrieve(const std::string& objectId) const;
};

}