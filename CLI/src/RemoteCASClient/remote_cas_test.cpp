#include "remote_cas_client.hpp"
#include "../storage/storage_manager.hpp"

#include <iostream>
#include <string>
#include <vector>

int main()
{
    try
    {
        Storage::StorageManager storage(
            ".aigit/cas"
        );

        storage.initialize();


        std::string objectId =
            storage.storeFile("test.txt");


        std::cout
            << "Local object ID: "
            << objectId
            << "\n";

        Remote::RemoteCASClient remote(
            "http://localhost:3000"
        );

        if (!remote.healthCheck())
        {
            std::cerr
                << "Remote backend is not reachable.\n";

            return 1;
        }

        std::cout
            << "Remote backend is reachable.\n";

        Remote::UploadNegotiation negotiation =
            remote.negotiateUpload(
                { objectId }
            );

        auto it =
            negotiation.uploadUrls.find(
                objectId
            );


        if (it ==
            negotiation.uploadUrls.end())
        {
            std::cout
                << "Remote already has object. "
                << "Upload skipped.\n";

            return 0;
        }


        std::string uploadUrl =
            it->second;


        std::string objectData =
            storage.retrieveFile(
                objectId
            );

        remote.uploadObject(
            uploadUrl,
            objectData
        );


        std::cout
            << "Object uploaded successfully.\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "Remote CAS test failed: "
            << e.what()
            << "\n";

        return 1;
    }
}