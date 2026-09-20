#include "gitutils.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace Utils
{
    std::string getBranchCommitHash(const std::string& branchName)
{
    // A branch with no commits yet has no ref file on disk; that's the
    // normal "no parent commit" state, not an error.
    fs::path branchPath = ".aigit/refs/heads/" + branchName;
    if(!fs::exists(branchPath))
    {
        return "";
    }

    std::ifstream branchFile(branchPath);

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

void writeBranchRef(const std::string& branchName, const std::string& commitHash)
{
    fs::path branchPath = fs::path(".aigit") / "refs" / "heads" / branchName;

    fs::create_directories(branchPath.parent_path());

    std::ofstream branchFile(branchPath, std::ios::trunc);
    if (!branchFile)
    {
        std::cerr << "Error: Could not update branch '" << branchName << "'.\n";
        return;
    }

    branchFile << commitHash;
}

void setHeadToBranch(const std::string& branchName)
{
    std::ofstream headFile(".aigit/HEAD", std::ios::trunc);
    if (!headFile.is_open())
    {
        std::cerr << "Error: Could not write to HEAD.\n";
        return;
    }

    headFile << "ref: refs/heads/" << branchName;
}

std::string normalizePath(const fs::path& p)
{
    std::string pathStr = p.generic_string();
    std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

    if (pathStr.rfind("./", 0) == 0)
    {
        pathStr = pathStr.substr(2);
    }

    while (!pathStr.empty() && (pathStr.back() == '\r' || pathStr.back() == '\n' || pathStr.back() == ' '))
    {
        pathStr.pop_back();
    }

    return pathStr;
}

bool isIgnoredPath(const std::string& normalizedPath)
{
    return normalizedPath.find(".aigit") != std::string::npos ||
           normalizedPath.find("build/") != std::string::npos ||
           normalizedPath.find(".git") != std::string::npos ||
           normalizedPath.find(".vscode/") != std::string::npos ||
           normalizedPath.find("vcpkg/") != std::string::npos;
}

}