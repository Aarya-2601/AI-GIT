#pragma once

#include <filesystem>
#include <string>

namespace Storage {

struct ObjectMetadata
{
    std::string objectId;
    long long size;
    std::string type;
    long long createdAt;
};

class MetadataDB
{
private:
    std::filesystem::path dbPath;

public:
    explicit MetadataDB(const std::filesystem::path& path);

    void initialize();

    void addObject(
        const std::string& objectId,
        long long size,
        const std::string& type
    );

    bool objectExists(
        const std::string& objectId
    ) const;

    ObjectMetadata getObject(
        const std::string& objectId
    ) const;
};

}