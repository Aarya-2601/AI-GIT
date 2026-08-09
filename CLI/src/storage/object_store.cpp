#include "object_store.hpp"
#include "../core/hashing.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>


namespace Storage {

ObjectStore::ObjectStore(const std::filesystem::path& root)
    : rootPath(root)
{
}

void ObjectStore::initialize()
{
    std::filesystem::create_directories(rootPath / "objects");
}

bool ObjectStore::exists(const std::string& objectId) const
{
    if (objectId.length() < 2) {
        return false;
    }

    std::filesystem::path objectPath =
        rootPath / "objects" /
        objectId.substr(0, 2) /
        objectId.substr(2);

    return std::filesystem::exists(objectPath);
}

std::string ObjectStore::store(const std::filesystem::path& filePath)
{
    std::ifstream input(filePath, std::ios::binary);

    if (!input.is_open()) {
        throw std::runtime_error(
            "Could not open file: " + filePath.string()
        );
    }

    std::stringstream buffer;
    buffer << input.rdbuf();

    std::string fileData = buffer.str();

    // Hash the RAW file contents.
    std::string objectId = Core::calcSHA256(fileData);

    if (objectId.empty()) {
        throw std::runtime_error(
            "SHA-256 hashing failed."
        );
    }

    std::filesystem::path objectPath =
        rootPath / "objects" /
        objectId.substr(0, 2) /
        objectId.substr(2);

    // Object already exists -> deduplication.
    if (std::filesystem::exists(objectPath)) {
        return objectId;
    }

    // Create the directory for this object.
    std::filesystem::create_directories(
        objectPath.parent_path()
    );

    std::ofstream output(objectPath, std::ios::binary);

    if (!output.is_open()) {
        throw std::runtime_error(
            "Could not create object: " + objectPath.string()
        );
    }

    output.write(fileData.data(), fileData.size());

    if (!output) {
        throw std::runtime_error(
            "Failed to write object."
        );
    }

    return objectId;
}

std::string ObjectStore::retrieve(const std::string& objectId) const
{
    if (objectId.length() < 2) {
        throw std::runtime_error("Invalid object ID.");
    }

    std::filesystem::path objectPath =
        rootPath / "objects" /
        objectId.substr(0, 2) /
        objectId.substr(2);

    if (!std::filesystem::exists(objectPath)) {
        throw std::runtime_error(
            "Object not found: " + objectId
        );
    }

    std::ifstream input(objectPath, std::ios::binary);

    if (!input.is_open()) {
        throw std::runtime_error(
            "Could not open object: " + objectId
        );
    }

    std::stringstream buffer;
    buffer << input.rdbuf();

    return buffer.str();
}

}