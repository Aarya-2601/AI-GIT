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

}
namespace Commands
{

int runCheckout(const std::string& branchName)
{
    return 0;
}

}