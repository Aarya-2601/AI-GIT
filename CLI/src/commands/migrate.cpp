#include "migrate.hpp"
#include "../storage/object_store.hpp"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace Commands
{

int runMigrate()
{
    if (!fs::exists(".aigit"))
    {
        std::cerr << "Error: Not an AI-Git repository." << std::endl;
        return 1;
    }

    Storage::ObjectStore objectStore(".aigit");

    Storage::ObjectStore::MigrationResult result =
        objectStore.migrateLegacyObjects();

    std::cout
        << "Migrated " << result.objectsMigrated
        << " legacy object(s); " << result.objectsAlreadyCurrent
        << " already up to date." << std::endl;

    if (result.failedObjectIds.empty())
    {
        return 0;
    }

    std::cerr
        << result.failedObjectIds.size()
        << " object(s) could not be read/migrated (see 'ai-git fsck' "
        << "for details):" << std::endl;

    for (const std::string& objectId : result.failedObjectIds)
    {
        std::cerr << "  " << objectId << std::endl;
    }

    return 1;
}

}
