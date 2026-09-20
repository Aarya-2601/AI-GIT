#include "checkout.hpp"
#include "../core/storage.hpp"
#include "../core/compression.hpp"
#include "../core/object_io.hpp"
#include "../core/index.hpp"
#include "../models/tree.hpp"
#include "../models/blob.hpp"
#include "../models/commit.hpp"
#include "../storage/storage_manager.hpp"
#include "../helpers/gitutils.hpp"

#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

namespace
{

void restoreTree(
    const Models::Tree& tree,
    const fs::path& destination
)
{
    Storage::StorageManager storageManager(".aigit");

    for (const auto& entry : tree.getEntries())
    {
        fs::path targetPath =
            destination / entry.name;

        if (entry.isSubtree)
        {
            fs::create_directories(targetPath);

            Models::Tree subtree =
                Core::loadTree(entry.hash);

            restoreTree(subtree, targetPath);
        }
        else
        {
            // Reassembles FastCDC chunks or monolithic blobs
            storageManager.restoreFile(entry.hash, targetPath);
        }
    }
}

void collectTrackedFiles(
    const Models::Tree& tree,
    const fs::path& currentPath,
    std::vector<fs::path>& files
)
{
    for (const auto& entry : tree.getEntries())
    {
        fs::path targetPath =
            currentPath / entry.name;

        if (entry.isSubtree)
        {
            Models::Tree subtree =
                Core::loadTree(entry.hash);

            collectTrackedFiles(
                subtree,
                targetPath,
                files
            );
        }
        else
        {
            files.push_back(targetPath);
        }
    }
}

void removeTrackedFiles(
    const std::vector<fs::path>& files
)
{
    for (const auto& file : files)
    {
        if (fs::exists(file) && fs::is_regular_file(file))
        {
            if (!fs::remove(file))
            {
                throw std::runtime_error(
                    "Failed to remove tracked file: " +
                    file.string()
                );
            }
        }
    }
}

// Rebuilds .aigit/index to exactly match `tree`, so the staging area
// reflects the branch just checked out rather than whatever was staged
// on the branch we switched away from.
void syncIndexToTree(
    const Models::Tree& tree,
    const fs::path& currentPath,
    Core::Index& index
)
{
    for (const auto& entry : tree.getEntries())
    {
        fs::path targetPath =
            currentPath / entry.name;

        if (entry.isSubtree)
        {
            Models::Tree subtree =
                Core::loadTree(entry.hash);

            syncIndexToTree(subtree, targetPath, index);
        }
        else
        {
            index.addEntry(
                Core::IndexEntry(
                    Utils::normalizePath(targetPath),
                    entry.hash,
                    entry.mode
                )
            );
        }
    }
}

}

namespace Commands
{

int runCheckout(const std::string& branchName)
{
    try
    {
        // 1. Check branch exists
        if (!Utils::branchExists(branchName))
        {
            std::cerr
                << "Error: Branch '"
                << branchName
                << "' does not exist."
                << std::endl;

            return 1;
        }

        // 2. Read target branch reference
        std::string targetCommitHash =
            Utils::getBranchCommitHash(branchName);

        if (targetCommitHash.empty())
        {
            throw std::runtime_error(
                "Branch does not point to a commit."
            );
        }

        // 3. Read target commit
        Models::Commit targetCommit =
            Core::loadCommit(targetCommitHash);

        // 4. Get target root tree
        std::string targetTreeHash =
            targetCommit.getTreeHash();

        // 5. Read target tree
        Models::Tree targetTree =
            Core::loadTree(targetTreeHash);

        // 6. Get current commit
        std::string currentCommitHash =
            Utils::getCurrentCommitHash();

        // 7. Remove currently tracked files
        if (!currentCommitHash.empty())
        {
            Models::Commit currentCommit =
                Core::loadCommit(currentCommitHash);

            std::string currentTreeHash =
                currentCommit.getTreeHash();

            Models::Tree currentTree =
                Core::loadTree(currentTreeHash);

            std::vector<fs::path> currentFiles;

            collectTrackedFiles(
                currentTree,
                ".",
                currentFiles
            );

            removeTrackedFiles(currentFiles);
        }

        // 8. Restore target snapshot
        restoreTree(
            targetTree,
            "."
        );

        // 9. Sync the index to the branch we just checked out, so
        // whatever was staged on the previous branch doesn't linger.
        Core::Index newIndex;
        syncIndexToTree(targetTree, ".", newIndex);
        newIndex.save(".aigit/index");

        Utils::setHeadToBranch(branchName);

        std::cout
            << "Checked out branch '"
            << branchName
            << "'."
            << std::endl;

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "Checkout error: "
            << e.what()
            << std::endl;

        return 1;
    }
}

}