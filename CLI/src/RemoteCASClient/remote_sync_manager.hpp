#pragma once

#include "remote_cas_client.hpp"
#include "../storage/storage_manager.hpp"

#include <string>

namespace Remote
{

class RemoteSyncManager
{
private:
    Storage::StorageManager& storageManager;
    RemoteCASClient& remoteClient;

public:
    RemoteSyncManager(
        Storage::StorageManager& storageManager,
        RemoteCASClient& remoteClient
    );

    void uploadManifest(
        const std::string& manifestId
    );
};

}