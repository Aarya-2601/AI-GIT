#pragma once

#include "remote_cas_client.hpp"
#include "../storage/storage_manager.hpp"

#include <string>
#include <vector>

namespace Remote
{

class RemoteSyncManager
{
private:
    Storage::StorageManager& storageManager;
    RemoteCASClient& remoteClient;

    void uploadObjects(
        const std::vector<std::string>& objectIds
    );

public:
    RemoteSyncManager(
        Storage::StorageManager& storageManager,
        RemoteCASClient& remoteClient
    );

    void syncObject(
        const std::string& objectId
    );
};

}