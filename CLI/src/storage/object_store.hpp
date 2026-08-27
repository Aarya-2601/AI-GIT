#pragma once

#include <filesystem>
#include <string>

namespace Storage
{

class ObjectStore
{
private:
    std::filesystem::path rootPath;

public:
    explicit ObjectStore(
        const std::filesystem::path& root
    );


    void initialize();

    bool exists(
        const std::string& objectId
    ) const;

    std::string store(
        const std::filesystem::path& filePath
    );


    void storeObject(
        const std::string& objectId,
        const std::string& data
    );

    // Retrieves the raw bytes of an object
    // using its SHA-256 object ID.
    std::string retrieve(
        const std::string& objectId
    ) const;
};

}