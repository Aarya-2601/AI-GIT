// Regression test: `checkout` must rewrite .aigit/index to match the
// branch being switched to.
//
// Before the fix: checkout.cpp restores the working tree files but never
// touches the index. A file staged (but not committed) on the old branch
// stays listed in the index after switching branches, even though it
// belongs to neither the old commit nor the new one -- `status`/`commit`
// on the new branch would then act on a phantom staged file.

#include "../commands/init.hpp"
#include "../commands/add.hpp"
#include "../commands/commit.hpp"
#include "../commands/branch.hpp"
#include "../commands/checkout.hpp"
#include "../core/index.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

int main()
{
    fs::path testDir =
        fs::temp_directory_path() / "aigit-checkout-index-sync-test";

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
        file << "committed on main";
    }
    Commands::runAdd(std::vector<std::string>{"a.txt"});

    if (Commands::runCommit("first commit") != 0)
    {
        std::cerr << "FAILED: first commit did not succeed.\n";
        return 1;
    }

    Commands::runBranch({"ai-git", "branch", "feature"});

    // Stage a new file on main, but never commit it.
    {
        std::ofstream file("d.txt");
        file << "staged but never committed";
    }
    Commands::runAdd(std::vector<std::string>{"d.txt"});

    if (Commands::runCheckout("feature") != 0)
    {
        std::cerr << "FAILED: checkout did not succeed.\n";
        return 1;
    }

    Core::Index index;
    index.load(".aigit/index");
    const auto& entries = index.getEntries();

    std::cout << "Index after checkout has " << entries.size() << " entrie(s):\n";
    for (const auto& [path, entry] : entries)
    {
        std::cout << "  " << path << "\n";
    }

    if (entries.find("d.txt") != entries.end())
    {
        std::cerr
            << "FAILED: index still lists 'd.txt', which was only staged "
            << "on main and does not belong to 'feature'.\n";
        return 1;
    }

    if (entries.find("a.txt") == entries.end())
    {
        std::cerr
            << "FAILED: index is missing 'a.txt', which 'feature' should "
            << "track.\n";
        return 1;
    }

    std::cout << "SUCCESS: index matches the checked-out branch's tree.\n";
    return 0;
}
