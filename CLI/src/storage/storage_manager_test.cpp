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

        // Store all three files
        std::string objectId1 =
            storage.storeFile("test.txt");

        std::string objectId2 =
            storage.storeFile("test1.txt");

        std::string objectId3 =
            storage.storeFile("test2.txt");

        // Print object IDs
        std::cout << "\n=== OBJECT IDs ===\n";

        std::cout
            << "test.txt: "
            << objectId1
            << "\n";

        std::cout
            << "test1.txt: "
            << objectId2
            << "\n";

        std::cout
            << "test2.txt: "
            << objectId3
            << "\n";

        // Check deduplication
        std::cout << "\n=== DEDUPLICATION TEST ===\n";

        std::cout
            << "test.txt == test1.txt: "
            << (objectId1 == objectId2 ? "YES" : "NO")
            << "\n";

        std::cout
            << "test.txt == test2.txt: "
            << (objectId1 == objectId3 ? "YES" : "NO")
            << "\n";

        std::cout
            << "test1.txt == test2.txt: "
            << (objectId2 == objectId3 ? "YES" : "NO")
            << "\n";

        // Check that the objects exist
        std::cout << "\n=== OBJECT EXISTENCE ===\n";

        std::cout
            << "test.txt object exists: "
            << (storage.objectExists(objectId1)
                    ? "YES"
                    : "NO")
            << "\n";

        std::cout
            << "test1.txt object exists: "
            << (storage.objectExists(objectId2)
                    ? "YES"
                    : "NO")
            << "\n";

        std::cout
            << "test2.txt object exists: "
            << (storage.objectExists(objectId3)
                    ? "YES"
                    : "NO")
            << "\n";

        // Retrieve the files
        std::cout << "\n=== RETRIEVAL ===\n";

        std::cout
            << "test.txt retrieved: "
            << storage.retrieveFile(objectId1)
            << "\n";

        std::cout
            << "test1.txt retrieved: "
            << storage.retrieveFile(objectId2)
            << "\n";

        std::cout
            << "test2.txt retrieved: "
            << storage.retrieveFile(objectId3)
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