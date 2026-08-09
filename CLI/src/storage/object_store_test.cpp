#include "object_store.hpp"
#include <iostream>
#include <string>

int main()
{
    try
    {
        Storage::ObjectStore store(".aigit");

        store.initialize();

        std::string id = store.store("test.txt");

        std::cout << "Object ID: " << id << "\n";

        std::cout << "Exists: "
                  << (store.exists(id) ? "YES" : "NO")
                  << "\n";

        std::string data = store.retrieve(id);

        std::cout << "Retrieved: " << data << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}