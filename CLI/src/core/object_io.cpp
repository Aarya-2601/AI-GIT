#include "object_io.hpp"

#include "../storage/object_store.hpp"

#include <stdexcept>

namespace Core
{

Models::Tree loadTree(const std::string& hash)
{
    Storage::ObjectStore objectStore(".aigit");

    std::string treeData = objectStore.retrieve(hash);

    return Models::Tree::deserialize(treeData);
}

Models::Commit loadCommit(const std::string& hash)
{
    Storage::ObjectStore objectStore(".aigit");

    std::string commitData = objectStore.retrieve(hash);

    return Models::Commit::deserialize(commitData);
}

}
