#include "fsck.hpp"
#include "../storage/metadata_db.hpp"
#include "../storage/object_store.hpp"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace Commands
{

int runFsck()
{
    if (!fs::exists(".aigit"))
    {
        std::cerr << "Error: Not an AI-Git repository." << std::endl;
        return 1;
    }

    Storage::ObjectStore objectStore(".aigit");

    std::size_t staleTmpRemoved = objectStore.cleanupStaleTmpFiles();

    if (staleTmpRemoved > 0)
    {
        std::cout
            << "Removed " << staleTmpRemoved
            << " stale .tmp file(s) left by an interrupted write."
            << std::endl;
    }

    Storage::MetadataDB metadataDB(".aigit/metadata.db");
    metadataDB.initialize();

    Storage::MetadataDB::RebuildResult result =
        metadataDB.rebuild(".aigit");

    std::cout
        << "Rebuilt metadata.db from disk: "
        << result.objectsRebuilt << " object(s) verified."
        << std::endl;

    if (result.corruptObjectIds.empty())
    {
        std::cout << "No corrupt objects found." << std::endl;
        return 0;
    }

    std::cerr
        << result.corruptObjectIds.size()
        << " corrupt object(s) found:" << std::endl;

    for (const std::string& objectId : result.corruptObjectIds)
    {
        std::cerr << "  " << objectId << std::endl;
    }

    return 1;
}

}
