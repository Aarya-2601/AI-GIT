#pragma once

#include "../models/tree.hpp"
#include "../models/commit.hpp"

#include <string>

// Shared "read compressed object -> decompress -> deserialize" helpers.
// Previously checkout.cpp, log.cpp and status.cpp each reimplemented this
// (status.cpp even hand-parsed the tree binary format instead of calling
// Models::Tree::deserialize). Centralizing it here means there is exactly
// one place that knows how a tree/commit object is read off disk.
namespace Core
{
    // Reads, decompresses and deserializes the tree object stored at
    // `hash`. Throws std::runtime_error if the object is missing or
    // malformed.
    Models::Tree loadTree(const std::string& hash);

    // Reads, decompresses and deserializes the commit object stored at
    // `hash`. Throws std::runtime_error if the object is missing or
    // malformed.
    Models::Commit loadCommit(const std::string& hash);
}
