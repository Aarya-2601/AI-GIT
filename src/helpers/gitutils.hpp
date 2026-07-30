#pragma once

#include <string>

namespace Utils
{
    std::string getCurrentBranchName();

    std::string getCurrentCommitHash();

    bool branchExists(const std::string& branchName);
}