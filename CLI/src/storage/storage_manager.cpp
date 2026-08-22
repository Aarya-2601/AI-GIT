#include "storage_manager.hpp"
#include "chunking.hpp"
#include "../core/filesystem.hpp"
#include "../core/hashing.hpp"

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

std::string StorageManager::readSlice(
    const std::filesystem::path& filePath,
    size_t offset,
    size_t length
) const
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error(
            "Failed to open file for slicing: " +
            filePath.string()
        );
    }

    file.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    std::string buffer(length, '\0');
    file.read(&buffer[0], static_cast<std::streamsize>(length));

    if (static_cast<size_t>(file.gcount()) != length)
    {
        throw std::runtime_error(
            "Failed to read complete chunk slice from: " +
            filePath.string()
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

    size_t totalBytes = std::filesystem::file_size(filePath);

    // ------------------------------------------------
    // FastCDC Chunking Branch for Large Files (> 256 KB)
    // ------------------------------------------------
    if (totalBytes > Chunking::MIN_SIZE)
    {
        std::vector<Chunk> chunks = Chunking::chunkFile(filePath.string());

        std::string manifestJson = "{\"type\":\"manifest\",\"total_size\":" +
                                   std::to_string(totalBytes) + ",\"chunks\":[";

        for (size_t i = 0; i < chunks.size(); ++i)
        {
            // 1. Extract raw binary slice
            std::string chunkData = readSlice(filePath, chunks[i].offset, chunks[i].length);

            // 2. Store raw chunk in CAS
            objectStore.storeObject(chunks[i].sha256, chunkData);

            // 3. Register chunk in SQLite
            metadataDB.addObject(
                chunks[i].sha256,
                static_cast<long long>(chunks[i].length),
                "chunk"
            );

            // 4. Build manifest entry
            manifestJson += "{\"hash\":\"" + chunks[i].sha256 + "\"," +
                            "\"size\":" + std::to_string(chunks[i].length) + "," +
                            "\"offset\":" + std::to_string(chunks[i].offset) + "}" +
                            (i + 1 < chunks.size() ? "," : "");
        }

        manifestJson += "]}";

        // 5. Store manifest itself in CAS
        std::string manifestId = Core::calcSHA256(manifestJson);
        objectStore.storeObject(manifestId, manifestJson);

        // 6. Record manifest metadata in SQLite
        metadataDB.addObject(
            manifestId,
            static_cast<long long>(manifestJson.size()),
            "manifest"
        );

        return manifestId;
    }

    // ------------------------------------------------
    // Monolithic File Branch (<= 256 KB)
    // ------------------------------------------------
    std::string objectId =
        objectStore.store(filePath);

    metadataDB.addObject(
        objectId,
        static_cast<long long>(totalBytes),
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

void StorageManager::restoreFile(
    const std::string& objectId,
    const std::filesystem::path& destinationPath
) const
{
    std::string data = objectStore.retrieve(objectId);

    // If object is a manifest, assemble chunks
    if (data.rfind("{\"type\":\"manifest\"", 0) == 0)
    {
        std::ofstream out(destinationPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
        {
            throw std::runtime_error(
                "Failed to open destination file: " +
                destinationPath.string()
            );
        }

        size_t pos = 0;
        while ((pos = data.find("\"hash\":\"", pos)) != std::string::npos)
        {
            pos += 8;
            size_t endPos = data.find("\"", pos);
            if (endPos == std::string::npos) break;

            std::string chunkHash = data.substr(pos, endPos - pos);
            std::string chunkBytes = objectStore.retrieve(chunkHash);
            out.write(chunkBytes.data(), static_cast<std::streamsize>(chunkBytes.size()));
            pos = endPos;
        }
        return;
    }

    // Monolithic file write
    std::ofstream out(destinationPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        throw std::runtime_error(
            "Failed to open destination file: " +
            destinationPath.string()
        );
    }
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

bool StorageManager::objectExists(
    const std::string& objectId
) const
{
    return objectStore.exists(objectId);
}

}