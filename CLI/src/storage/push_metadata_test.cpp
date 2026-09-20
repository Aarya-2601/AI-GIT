// Regression test: after `add` + `commit`, every object written to the
// repository (blob, tree, commit) must be registered in MetadataDB.
// `push` only reads MetadataDB::getAllObjectIds() to decide what to
// upload, so any object missing here is an object that silently never
// reaches the remote -- a pushed repo would be missing its whole history.
//
// Before the fix: only the blob (written via StorageManager/ObjectStore)
// is registered. Tree and commit objects are written via the legacy
// Core::Storage path, which never touches MetadataDB, so they're absent.

#include "../commands/init.hpp"
#include "../commands/add.hpp"
#include "../commands/commit.hpp"
#include "metadata_db.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

int main()
{
    fs::path testDir =
        fs::temp_directory_path() / "aigit-push-metadata-test";

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
        file << "hello world";
    }

    Commands::runAdd(std::vector<std::string>{"a.txt"});

    if (Commands::runCommit("test commit") != 0)
    {
        std::cerr << "FAILED: commit did not succeed.\n";
        return 1;
    }

    Storage::MetadataDB metadataDB(".aigit/metadata.db");
    std::vector<std::string> objectIds = metadataDB.getAllObjectIds();

    // One file at the repo root, no subdirectories, produces exactly:
    // 1 blob + 1 (root) tree + 1 commit = 3 objects.
    std::cout
        << "MetadataDB tracks " << objectIds.size()
        << " object(s) after add+commit (expected 3: blob, tree, commit).\n";

    if (objectIds.size() != 3)
    {
        std::cerr
            << "FAILED: tree and/or commit objects are not registered in "
            << "MetadataDB -- `push` would silently skip them.\n";
        return 1;
    }

    std::cout << "SUCCESS: blob, tree and commit are all push-visible.\n";
    return 0;
}
