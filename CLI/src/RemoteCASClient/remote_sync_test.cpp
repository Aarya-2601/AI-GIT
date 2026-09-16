#include "remote_cas_client.hpp"
#include "remote_sync_manager.hpp"

#include "../storage/storage_manager.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr
            << "Usage: remote-sync-test <object-id> [server-url]"
            << std::endl;

        return 1;
    }

    const std::string objectId = argv[1];

    const std::string serverUrl =
        (argc >= 3)
            ? argv[2]
            : "http://localhost:3000";

    try
    {
        Storage::StorageManager storageManager(
    ".aigit/cas"
);

        Remote::RemoteCASClient remoteClient(
            serverUrl
        );

        Remote::RemoteSyncManager syncManager(
            storageManager,
            remoteClient
        );

        std::cout
            << "[Remote Test] Server: "
            << serverUrl
            << std::endl;

        std::cout
            << "[Remote Test] Object: "
            << objectId
            << std::endl;

        std::cout
            << "[Remote Test] Checking server..."
            << std::endl;

        if (!remoteClient.healthCheck())
        {
            std::cerr
                << "[Remote Test] Server is not reachable."
                << std::endl;

            return 1;
        }

        std::cout
            << "[Remote Test] Server reachable."
            << std::endl;

        std::cout
            << "[Remote Test] Starting synchronization..."
            << std::endl;

        syncManager.syncObject(
            objectId
        );

        std::cout
            << "[Remote Test] Synchronization complete."
            << std::endl;
    }
    catch (const std::exception& error)
    {
        std::cerr
            << "[Remote Test] Error: "
            << error.what()
            << std::endl;

        return 1;
    }

    return 0;
}