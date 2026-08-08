#include "object_store.hpp"
#include <iostream>

int main()
{
    Storage::ObjectStore store(".ai-store");

    store.initialize();

    std::cout << "Object store initialized.\n";

    return 0;
}