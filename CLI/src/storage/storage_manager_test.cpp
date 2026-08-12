#include "storage_manager.hpp"

#include <iostream>
#include <string>

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
            << "Object ID: "
            << objectId
            << "\n";

        std::cout
            << "Exists: "
            << (
                storage.objectExists(objectId)
                    ? "YES"
                    : "NO"
            )
            << "\n";

        std::string data =
            storage.retrieveFile(objectId);

        std::cout
            << "Retrieved: "
            << data
            << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "Error: "
            << e.what()
            << "\n";

        return 1;
    }

    return 0;
}