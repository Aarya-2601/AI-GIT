#pragma once

#include <string>

namespace Models
{
    struct Commit
    {
        std::string treeHash;
        std::string parentHash;

        std::string author;
        std::string committer;

        std::string message;
    };
}