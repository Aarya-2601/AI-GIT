#pragma once

#include <string>

namespace Utils
{
    std::string getBranchCommitHash(const std::string& branchName);
    std::string getCurrentBranchName();

    std::string getCurrentCommitHash();

   

    bool branchExists(const std::string& branchName);
}