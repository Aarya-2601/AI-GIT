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

void RemoteSyncManager::uploadObjects(
    const std::vector<std::string>& objectIds
)
{
    UploadNegotiation negotiation =
        remoteClient.negotiateUpload(
            objectIds
        );

    for (
        const auto& [objectId, uploadUrl] :
        negotiation.uploadUrls
    )
    {
        if (!storageManager.objectExists(objectId))
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

void RemoteSyncManager::syncObject(
    const std::string& objectId
)
{
    if (!storageManager.objectExists(objectId))
    {
        throw std::runtime_error(
            "Local CAS object not found: " +
            objectId
        );
    }

    std::string objectData =
        storageManager.retrieveFile(
            objectId
        );

    nlohmann::json parsedObject;

    bool isManifest = false;

    try
    {
        parsedObject =
            nlohmann::json::parse(
                objectData
            );

        isManifest =
            parsedObject.is_object() &&
            parsedObject.value("type", "") == "manifest" &&
            parsedObject.contains("chunks") &&
            parsedObject["chunks"].is_array();
    }
    catch (const nlohmann::json::parse_error&)
    {
        isManifest = false;
    }

    // Normal CAS object:
    // only this object needs to be synchronized.
    if (!isManifest)
    {
        uploadObjects(
            {objectId}
        );

        return;
    }

  
    std::vector<std::string> objectIds;

    for (const auto& chunk : parsedObject["chunks"])
    {
        if (
            !chunk.is_object() ||
            !chunk.contains("hash") ||
            !chunk["hash"].is_string()
        )
        {
            throw std::runtime_error(
                "Invalid chunk entry in manifest: " +
                objectId
            );
        }

        std::string chunkId =
            chunk["hash"].get<std::string>();

        if (!storageManager.objectExists(chunkId))
        {
            throw std::runtime_error(
                "Manifest references missing local chunk: " +
                chunkId
            );
        }

        objectIds.push_back(
            chunkId
        );
    }
    objectIds.push_back(
        objectId
    );

    uploadObjects(
        objectIds
    );
}

}