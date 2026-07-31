#include "../commands/checkout.hpp"
#include "../core/storage.hpp"
#include "../core/compression.hpp"

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

}


namespace Commands
{

int runCheckout(const std::string& branchName)
{
    return 0;
}

}