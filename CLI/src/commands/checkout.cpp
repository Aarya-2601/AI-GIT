#include "../commands/checkout.hpp"
#include "../core/storage.hpp"
#include "../core/compression.hpp"
#include "checkout.hpp"

#include "../core/storage.hpp"
#include "../core/compression.hpp"
#include "../models/tree.hpp"

#include "../models/blob.hpp"

#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>
#include "../helpers/gitutils.hpp"

namespace 
{

Models::Commit readCommit(const std::string& commitHash)
{
    std::string compressedObject =
        Core::Storage::readObject(commitHash);

    if (compressedObject.empty())
    {
        throw std::runtime_error("Failed to read commit object.");
    }

    std::string objectData =
        Core::decompressData(compressedObject);

    return Models::Commit::deserialize(objectData);
}

Models::Tree readTree(const std::string& treeHash)
{
    std::string compressedObject =
        Core::Storage::readObject(treeHash);

    if (compressedObject.empty())
    {
        throw std::runtime_error(
            "Failed to read tree object: " + treeHash
        );
    }

    std::string treeData =Core::decompressData(compressedObject);

    return Models::Tree::deserialize(treeData);
}

Models::Blob readBlob(const std::string& blobHash)
{
    std::string compressedObject =
        Core::Storage::readObject(blobHash);

    if (compressedObject.empty())
    {
        throw std::runtime_error(
            "Failed to read blob object: " + blobHash
        );
    }

    std::string blobData =
        Core::decompressData(compressedObject);

    return Models::Blob::deserialize(blobData);
}

void restoreTree(
    const Models::Tree& tree,
    const fs::path& destination
)
{
    for (const auto& entry : tree.getEntries())
    {
        fs::path targetPath =
            destination / entry.name;

        if (entry.isSubtree)
        {
            fs::create_directories(targetPath);

            Models::Tree subtree =
                readTree(entry.hash);

            restoreTree(subtree, targetPath);
        }
        else
        {
            Models::Blob blob =
                readBlob(entry.hash);

            std::ofstream outFile(
                targetPath,
                std::ios::binary
            );

            if (!outFile.is_open())
            {
                throw std::runtime_error(
                    "Failed to create file: " +
                    targetPath.string()
                );
            }

            outFile << blob.getContent();

            outFile.close();
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
                readTree(entry.hash);

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

void updateHEAD(const std::string& branchName)
{
    std::ofstream headFile(
        ".aigit/HEAD",
        std::ios::trunc
    );

    if (!headFile.is_open())
    {
        throw std::runtime_error(
            "Failed to open HEAD for writing."
        );
    }

    headFile << "ref: refs/heads/"
             << branchName;

    headFile.close();
}

}
namespace Commands
{
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
        std::ifstream branchFile(
            ".aigit/refs/heads/" + branchName
        );

        if (!branchFile.is_open())
        {
            throw std::runtime_error(
                "Failed to open branch reference."
            );
        }

        std::string targetCommitHash;
        std::getline(branchFile, targetCommitHash);
        branchFile.close();

        if (targetCommitHash.empty())
        {
            throw std::runtime_error(
                "Branch does not point to a commit."
            );
        }

        // 3. Read target commit
        Models::Commit targetCommit =
            readCommit(targetCommitHash);

        // 4. Get target root tree
        std::string targetTreeHash =
            targetCommit.getTreeHash();

        // 5. Read target tree
        Models::Tree targetTree =
            readTree(targetTreeHash);

        // 6. Get current commit
        std::string currentCommitHash =
            Utils::getCurrentCommitHash();

        // 7. Remove currently tracked files
        if (!currentCommitHash.empty())
        {
            Models::Commit currentCommit =
                readCommit(currentCommitHash);

            std::string currentTreeHash =
                currentCommit.getTreeHash();

            Models::Tree currentTree =
                readTree(currentTreeHash);

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

        updateHEAD(branchName);

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
}