// Regression test for design step 9: MetadataDB::rebuild() must
// reconstruct the objects table from a disk walk (blob(s) + tree +
// commit), reading each object's type from its on-disk header, and must
// report -- not hide -- any object that fails hash verification.

#include "../commands/init.hpp"
#include "../commands/add.hpp"
#include "../commands/commit.hpp"
#include "../storage/metadata_db.hpp"
#include "../storage/object_store.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

int main()
{
    fs::path testDir =
        fs::temp_directory_path() / "aigit-metadata-rebuild-test";

    std::error_code ec;
    fs::remove_all(testDir, ec);
    fs::create_directories(testDir);
    fs::current_path(testDir);

    if (Commands::runInit() != 0)
    {
        std::cerr << "FAILED: init did not succeed.\n";
        return 1;
    }

    {
        std::ofstream file("a.txt");
        file << "metadata rebuild fixture a";
    }
    {
        std::ofstream file("b.txt");
        file << "metadata rebuild fixture b";
    }

    Commands::runAdd(std::vector<std::string>{"a.txt", "b.txt"});

    if (Commands::runCommit("rebuild fixture commit") != 0)
    {
        std::cerr << "FAILED: commit did not succeed.\n";
        return 1;
    }

    // Simulate metadata.db loss/corruption: wipe it and reinitialize an
    // empty one, exactly like a user recovering from a deleted/corrupted
    // metadata.db would.
    fs::remove(".aigit/metadata.db");

    Storage::MetadataDB metadataDB(".aigit/metadata.db");
    metadataDB.initialize();

    Storage::MetadataDB::RebuildResult result =
        metadataDB.rebuild(".aigit");

    // 2 blobs + 1 tree + 1 commit.
    if (result.objectsRebuilt != 4)
    {
        std::cerr
            << "FAILED: expected 4 objects rebuilt (2 blobs + tree + "
            << "commit), got " << result.objectsRebuilt << ".\n";
        return 1;
    }

    if (!result.corruptObjectIds.empty())
    {
        std::cerr
            << "FAILED: rebuild reported "
            << result.corruptObjectIds.size()
            << " corrupt object(s) on an intact repo.\n";
        return 1;
    }

    std::vector<std::string> allIds = metadataDB.getAllObjectIds();

    bool sawTree = false;
    bool sawCommit = false;

    for (const std::string& id : allIds)
    {
        Storage::ObjectMetadata meta = metadataDB.getObject(id);

        if (meta.type == "tree")
        {
            sawTree = true;
        }
        else if (meta.type == "commit")
        {
            sawCommit = true;
        }
    }

    if (!sawTree || !sawCommit)
    {
        std::cerr
            << "FAILED: rebuild did not record a 'tree' and a 'commit' "
            << "typed object (type byte not read back correctly).\n";
        return 1;
    }

    std::cout
        << "SUCCESS: rebuild reconstructed " << result.objectsRebuilt
        << " objects from disk with correct types, no false corruption.\n";

    // Now corrupt one on-disk object and confirm rebuild reports it
    // instead of silently dropping it or aborting the whole walk.

    Storage::ObjectStore objectStore(".aigit");
    std::string corruptId = allIds.front();

    fs::path corruptPath =
        fs::path(".aigit") / "objects" /
        corruptId.substr(0, 2) / corruptId.substr(2);

    {
        std::fstream f(
            corruptPath,
            std::ios::binary | std::ios::in | std::ios::out
        );
        f.seekp(0);
        char flipped = 'Z';
        f.write(&flipped, 1);
    }

    Storage::MetadataDB::RebuildResult afterCorruption =
        metadataDB.rebuild(".aigit");

    if (afterCorruption.objectsRebuilt != 3)
    {
        std::cerr
            << "FAILED: expected 3 valid objects after corrupting one, "
            << "got " << afterCorruption.objectsRebuilt << ".\n";
        return 1;
    }

    if (
        afterCorruption.corruptObjectIds.size() != 1 ||
        afterCorruption.corruptObjectIds[0] != corruptId
    )
    {
        std::cerr
            << "FAILED: rebuild did not report the corrupted object ("
            << corruptId << ") as corrupt.\n";
        return 1;
    }

    std::cout
        << "SUCCESS: rebuild reports a corrupted on-disk object instead "
        << "of hiding it or aborting the walk.\n";

    return 0;
}
