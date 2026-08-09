#pragma once

#include <string>
#include <filesystem>

namespace Storage {

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
};

}