#include "remote_sync_manager.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <vector>

namespace Remote
{

RemoteSyncManager::RemoteSyncManager(
    Storage::StorageManager& storageManager,
    RemoteCASClient& remoteClient
)
    : storageManager(storageManager),
      remoteClient(remoteClient)
{
}

void RemoteSyncManager::uploadManifest(
    const std::string& manifestId
)
{
    if (!storageManager.objectExists(manifestId))
    {
        throw std::runtime_error(
            "Local manifest not found: " +
            manifestId
        );
    }

    std::string manifestData =
        storageManager.retrieveFile(
            manifestId
        );

    nlohmann::json manifest =
        nlohmann::json::parse(
            manifestData
        );

    if (
        !manifest.is_object() ||
        manifest.value("type", "") != "manifest" ||
        !manifest.contains("chunks") ||
        !manifest["chunks"].is_array()
    )
    {
        throw std::runtime_error(
            "Invalid manifest object."
        );
    }

    std::vector<std::string> objectIds;

    for (const auto& chunk : manifest["chunks"])
    {
        std::string chunkId =
            chunk.at("hash")
                .get<std::string>();

        objectIds.push_back(
            chunkId
        );
    }

    objectIds.push_back(
        manifestId
    );

    UploadNegotiation negotiation =
        remoteClient.negotiateUpload(
            objectIds
        );

    for (
        const auto& [objectId, uploadUrl] :
        negotiation.uploadUrls
    )
    {
        if (
            !storageManager.objectExists(
                objectId
            )
        )
        {
            throw std::runtime_error(
                "Local CAS object missing: " +
                objectId
            );
        }

        std::string objectData =
            storageManager.retrieveFile(
                objectId
            );

        remoteClient.uploadObject(
            uploadUrl,
            objectData
        );
    }
}

}