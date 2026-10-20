// Regression test for design step 11: gc must delete objects unreachable
// from any ref or the index, and must never delete anything that is
// reachable -- including objects only reachable through older commits in
// history, and objects only reachable through the staging index (added,
// not yet committed).

#include "../commands/init.hpp"
#include "../commands/add.hpp"
#include "../commands/commit.hpp"
#include "../commands/gc.hpp"
#include "../storage/object_store.hpp"
#include "../storage/metadata_db.hpp"
#include "../core/hashing.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

int main()
{
    fs::path testDir = fs::temp_directory_path() / "aigit-gc-test";

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
        file << "gc fixture: committed file a";
    }

    // The first commit's blob for a.txt (small file, raw-bytes hash --
    // same scheme StorageManager::storeFile uses for content under
    // Chunking::MIN_SIZE).
    std::string firstBlobId =
        Core::calcSHA256("gc fixture: committed file a");

    Commands::runAdd(std::vector<std::string>{"a.txt"});

    if (Commands::runCommit("first commit") != 0)
    {
        std::cerr << "FAILED: first commit did not succeed.\n";
        return 1;
    }

    // A second commit that changes a.txt -- the FIRST commit's blob/tree/
    // commit objects are now only reachable through history, not through
    // the current tree directly. gc must still keep them.
    {
        std::ofstream file("a.txt");
        file << "gc fixture: committed file a, modified";
    }

    std::string secondBlobId =
        Core::calcSHA256("gc fixture: committed file a, modified");

    Commands::runAdd(std::vector<std::string>{"a.txt"});

    if (Commands::runCommit("second commit") != 0)
    {
        std::cerr << "FAILED: second commit did not succeed.\n";
        return 1;
    }

    // A staged-but-uncommitted file -- its blob is reachable only through
    // the index, no commit/tree references it yet.
    {
        std::ofstream file("staged.txt");
        file << "gc fixture: staged but not committed";
    }

    Commands::runAdd(std::vector<std::string>{"staged.txt"});

    std::string stagedBlobId; // read back from the index below

    // An orphan object: never referenced by any commit, tree, or the
    // index. This is what gc is supposed to remove.
    Storage::ObjectStore objectStore(".aigit");
    std::string orphanContent = "gc fixture: orphaned object, nothing points to this";
    std::string orphanId = Core::calcSHA256(orphanContent);
    objectStore.storeObject(orphanId, orphanContent, "chunk");

    if (!objectStore.exists(orphanId))
    {
        std::cerr << "FAILED: could not set up the orphan fixture.\n";
        return 1;
    }

    // Read the staged blob's real ID back from the index.
    {
        std::ifstream indexFile(".aigit/index");
        std::string line;
        bool found = false;

        while (std::getline(indexFile, line))
        {
            if (line.find("staged.txt") != std::string::npos)
            {
                std::istringstream iss(line);
                std::string mode, hash, path;
                iss >> mode >> hash >> path;
                stagedBlobId = hash;
                found = true;
                break;
            }
        }

        if (!found)
        {
            std::cerr << "FAILED: staged.txt not found in index.\n";
            return 1;
        }
    }

    if (Commands::runGc() != 0)
    {
        std::cerr << "FAILED: gc did not succeed.\n";
        return 1;
    }

    if (objectStore.exists(orphanId))
    {
        std::cerr
            << "FAILED: gc did not remove the unreachable orphan object.\n";
        return 1;
    }

    Storage::MetadataDB metadataDB(".aigit/metadata.db");

    if (metadataDB.objectExists(orphanId))
    {
        std::cerr
            << "FAILED: gc removed the orphan on disk but left its "
            << "MetadataDB row behind.\n";
        return 1;
    }

    if (!objectStore.exists(stagedBlobId))
    {
        std::cerr
            << "FAILED: gc deleted a blob only reachable through the "
            << "staging index.\n";
        return 1;
    }

    // The whole point of mark-and-sweep: content reachable through a
    // ref's commit tree (current tree AND older history) must survive.
    if (!objectStore.exists(secondBlobId))
    {
        std::cerr
            << "FAILED: gc deleted a blob reachable through the "
            << "current commit's tree.\n";
        return 1;
    }

    if (!objectStore.exists(firstBlobId))
    {
        std::cerr
            << "FAILED: gc deleted a blob only reachable through an "
            << "older commit's tree (history), not the current tree.\n";
        return 1;
    }

    std::cout
        << "SUCCESS: gc removed the unreachable orphan object and its "
        << "metadata row, while keeping the staged-but-uncommitted "
        << "blob and every blob reachable through commit history.\n";

    return 0;
}
