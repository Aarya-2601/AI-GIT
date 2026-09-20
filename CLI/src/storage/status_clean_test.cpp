// Regression test: `status` must report a clean working tree for a file
// that hasn't changed since the last commit.
//
// Before the fix: status.cpp hashes disk content via
// Models::Blob::serialize() + SHA-256 (a git-style "blob <size>\0<data>"
// wrapper), while `add` stores the object via StorageManager, which hashes
// raw bytes directly (no wrapper) for small files, or a chunk manifest for
// large ones. Those two hashes can never agree, so every tracked file is
// reported "modified" immediately after every commit.

#include "../commands/init.hpp"
#include "../commands/add.hpp"
#include "../commands/commit.hpp"
#include "../commands/status.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

int main()
{
    fs::path testDir =
        fs::temp_directory_path() / "aigit-status-clean-test";

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
        file << "hello world, unchanged after commit";
    }

    Commands::runAdd(std::vector<std::string>{"a.txt"});

    if (Commands::runCommit("initial commit") != 0)
    {
        std::cerr << "FAILED: commit did not succeed.\n";
        return 1;
    }

    std::ostringstream captured;
    std::streambuf* oldBuf = std::cout.rdbuf(captured.rdbuf());
    Commands::runStatus();
    std::cout.rdbuf(oldBuf);

    std::string output = captured.str();
    std::cout << "--- captured status output ---\n" << output << "-------------------------------\n";

    if (output.find("modified:") != std::string::npos)
    {
        std::cerr
            << "FAILED: status reports an unchanged file as modified "
            << "right after commit.\n";
        return 1;
    }

    std::cout << "SUCCESS: status reports a clean working tree after commit.\n";
    return 0;
}
