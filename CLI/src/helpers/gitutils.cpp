#include "gitutils.hpp"

#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace Utils
{
    std::string getBranchCommitHash(const std::string& branchName)
{
    std::ifstream branchFile(".aigit/refs/heads/" + branchName);

    if(!branchFile)
    {
        std::cerr << "Error: Unable to open branch reference.\n";
        return "";
    }

    std::string commitHash;
    std::getline(branchFile, commitHash);

    return commitHash;
}

    // basically finding the last of the / in the ref path and returning the substring after it

std::string getCurrentBranchName()
{
    std::ifstream headFile(".aigit/HEAD");

    if(!headFile)
    {
        std::cerr << "Error: Unable to open HEAD.\n";
        return "";
    }

    std::string line;
    std::getline(headFile, line);

    const std::string prefix = "ref: ";

    if(line.substr(0, prefix.size()) != prefix)
    {
        std::cerr << "Error: Invalid HEAD format.\n";
        return "";
    }

    std::string refPath = line.substr(prefix.size());

    size_t pos = refPath.find_last_of('/');

    if(pos == std::string::npos)
    {
        std::cerr << "Error: Invalid reference path.\n";
        return "";
    }

    return refPath.substr(pos + 1);
}

std::string getCurrentCommitHash()
{
    std::string branch = getCurrentBranchName();

    if(branch.empty())
        return "";

    return getBranchCommitHash(branch);
}

bool branchExists(const std::string& branchName)
{
    fs::path branchPath = ".aigit/refs/heads/" + branchName;

    return fs::exists(branchPath);
}

}