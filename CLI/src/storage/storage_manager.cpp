#include "storage_manager.hpp"

#include "../core/hashing.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <vector>

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

std::string StorageManager::readSlice(
    const std::filesystem::path& filePath,
    size_t offset,
    size_t length
) const
{
    std::ifstream file(
        filePath,
        std::ios::binary
    );

    if (!file.is_open())
    {
        throw std::runtime_error(
            "Failed to open file: " +
            filePath.string()
        );
    }

    file.seekg(
        static_cast<std::streamoff>(offset),
        std::ios::beg
    );

    std::string buffer(length, '\0');

    file.read(
        buffer.data(),
        static_cast<std::streamsize>(length)
    );

    if (
        static_cast<size_t>(file.gcount())
        != length
    )
    {
        throw std::runtime_error(
            "Failed to read complete chunk."
        );
    }

    return buffer;
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

    size_t totalBytes =
        std::filesystem::file_size(filePath);

    if (totalBytes <= Chunking::MIN_SIZE)
    {
        std::string objectId =
            objectStore.store(filePath);

        metadataDB.addObject(
            objectId,
            static_cast<long long>(totalBytes),
            "file"
        );

        return objectId;
    }

    std::vector<Chunk> chunks =
        Chunking::chunkFile(
            filePath.string()
        );

    nlohmann::json manifest;

    manifest["type"] = "manifest";
    manifest["total_size"] = totalBytes;
    manifest["chunks"] =
        nlohmann::json::array();

    for (const Chunk& chunk : chunks)
    {
        if (!objectStore.exists(chunk.sha256))
        {
            std::string chunkData =
                readSlice(
                    filePath,
                    chunk.offset,
                    chunk.length
                );

            objectStore.storeObject(
                chunk.sha256,
                chunkData
            );

            metadataDB.addObject(
                chunk.sha256,
                static_cast<long long>(
                    chunk.length
                ),
                "chunk"
            );
        }

        manifest["chunks"].push_back(
            {
                {"hash", chunk.sha256},
                {"size", chunk.length},
                {"offset", chunk.offset}
            }
        );
    }

    std::string manifestData =
        manifest.dump();

    std::string manifestId =
        Core::calcSHA256(
            manifestData
        );

    if (!objectStore.exists(manifestId))
    {
        objectStore.storeObject(
            manifestId,
            manifestData
        );

        metadataDB.addObject(
            manifestId,
            static_cast<long long>(
                manifestData.size()
            ),
            "manifest"
        );
    }

    return manifestId;
}

std::string StorageManager::retrieveFile(
    const std::string& objectId
) const
{
    return objectStore.retrieve(
        objectId
    );
}

void StorageManager::restoreFile(
    const std::string& objectId,
    const std::filesystem::path& destinationPath
) const
{
    std::string data =
        objectStore.retrieve(
            objectId
        );

    std::ofstream output(
        destinationPath,
        std::ios::binary |
        std::ios::trunc
    );

    if (!output.is_open())
    {
        throw std::runtime_error(
            "Failed to open destination file: " +
            destinationPath.string()
        );
    }

    nlohmann::json manifest =
        nlohmann::json::parse(
            data,
            nullptr,
            false
        );

    if (
        !manifest.is_discarded() &&
        manifest.is_object() &&
        manifest.value("type", "") == "manifest"
    )
    {
        if (
            !manifest.contains("chunks") ||
            !manifest["chunks"].is_array()
        )
        {
            throw std::runtime_error(
                "Invalid manifest."
            );
        }

        for (const auto& chunk : manifest["chunks"])
        {
            std::string chunkHash =
                chunk.at("hash")
                    .get<std::string>();

            std::string chunkData =
                objectStore.retrieve(
                    chunkHash
                );

            output.write(
                chunkData.data(),
                static_cast<std::streamsize>(
                    chunkData.size()
                )
            );

            if (!output)
            {
                throw std::runtime_error(
                    "Failed while reconstructing file."
                );
            }
        }

        return;
    }

    output.write(
        data.data(),
        static_cast<std::streamsize>(
            data.size()
        )
    );

    if (!output)
    {
        throw std::runtime_error(
            "Failed to restore file."
        );
    }
}

bool StorageManager::objectExists(
    const std::string& objectId
) const
{
    return objectStore.exists(
        objectId
    );
}

}