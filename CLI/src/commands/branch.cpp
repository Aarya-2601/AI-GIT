#include "branch.hpp"
#include <bits/stdc++.h>

#include "../helpers/gitutils.hpp"

#include <fstream>
#include <iostream>

namespace Commands
{

void runBranch(const std::vector<std::string>& args)
{
    // Expected:
    // aigit branch feature

    if(args.size() != 3)
    {
        std::cout << "Usage: aigit branch <branch-name>\n";
        return;
    }

    std::string branchName = args[2];

    if(Utils::branchExists(branchName))
    {
        std::cout << "Branch '" << branchName << "' already exists.\n";
        return;
    }

    std::string currentCommit = Utils::getCurrentCommitHash();

    if(currentCommit.empty())
    {
        std::cout << "No current commit found.\n";
        return;
    }

    std::ofstream branchFile(".aigit/refs/heads/" + branchName);

    if(!branchFile)
    {
        std::cout << "Unable to create branch.\n";
        return;
    }

    branchFile << currentCommit;

    branchFile.close();

    std::cout << "Created branch '" << branchName << "'\n";
}

#include <filesystem>

namespace fs = std::filesystem;

void listBranches()
{
    std::string currentBranch = Utils::getCurrentBranchName();

    fs::path branchesPath(".aigit/refs/heads");

    if(!fs::exists(branchesPath))
    {
        std::cout << "No branches found.\n";
        return;
    }

    for(const auto& entry : fs::directory_iterator(branchesPath))
    {
        std::string branchName = entry.path().filename().string();

        if(branchName == currentBranch)
            std::cout << "* " << branchName << '\n';
        else
            std::cout << "  " << branchName << '\n';
    }
}

}