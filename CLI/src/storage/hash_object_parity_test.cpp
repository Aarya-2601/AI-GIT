// Regression test for design step 6: `hash-object` must print the same
// object ID that `add` would record for identical content -- both go
// through StorageManager::storeFile now (raw-bytes / chunk-manifest
// hashing, no "blob <n>\0" wrapper). Before this step, hash-object hashed
// Models::Blob::serialize()'s wrapped payload while add hashed raw bytes,
// so the two never agreed.

#include "../commands/init.hpp"
#include "../commands/add.hpp"
#include "../commands/commands.hpp"
#include "../core/index.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

int main()
{
    fs::path testDir =
        fs::temp_directory_path() / "aigit-hash-object-parity-test";

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
        std::ofstream file("f.txt");
        file << "hash-object/add parity fixture content";
    }

    std::ostringstream captured;
    std::streambuf* oldBuf = std::cout.rdbuf(captured.rdbuf());
    int rc = Commands::runHashObject("f.txt");
    std::cout.rdbuf(oldBuf);

    if (rc != 0)
    {
        std::cerr << "FAILED: hash-object did not succeed.\n";
        return 1;
    }

    std::string hashObjectId = captured.str();
    while (!hashObjectId.empty() &&
           (hashObjectId.back() == '\n' || hashObjectId.back() == '\r'))
    {
        hashObjectId.pop_back();
    }

    if (Commands::runAdd(std::vector<std::string>{"f.txt"}) != 0)
    {
        std::cerr << "FAILED: add did not succeed.\n";
        return 1;
    }

    Core::Index index;
    index.load(".aigit/index");

    const auto& entries = index.getEntries();
    auto it = entries.find("f.txt");

    if (it == entries.end())
    {
        std::cerr << "FAILED: f.txt not found in index after add.\n";
        return 1;
    }

    std::cout << "hash-object printed: " << hashObjectId << "\n";
    std::cout << "add recorded:        " << it->second.hash << "\n";

    if (hashObjectId != it->second.hash)
    {
        std::cerr
            << "FAILED: hash-object and add disagree on the object ID "
            << "for identical content.\n";
        return 1;
    }

    std::cout
        << "SUCCESS: hash-object and add agree on the object ID for "
        << "identical content.\n";

    return 0;
}
