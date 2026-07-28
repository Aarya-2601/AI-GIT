#include <bits/stdc++.h>

#include "../commands/log.hpp"

#include "../models/commitstruct.hpp"

namespace fs=std::filesystem;

using namespace std;

//theres no input in this everythings from head and refs folders 

namespace
{
    // first thing is to know where the latest pointer is at (which branch) and get the latest commit hash
    std::string getCurrentBranchRef()
{
    std::ifstream headFile(".aigit/HEAD");

    if (!headFile.is_open())
    {
        throw std::runtime_error("Failed to open HEAD.");
    }

    std::string refLine;
    std::getline(headFile, refLine);
    headFile.close();

    if (refLine.substr(0, 5) != "ref: ")
    {
        throw std::runtime_error("Invalid HEAD format.");
    }

    return refLine.substr(5);
}

std::string getCurrentCommitHash(const std::string& branchRef)
{
    std::ifstream branchFile(".aigit/" + branchRef);

    if (!branchFile.is_open())
    {
        throw std::runtime_error("Failed to open branch.");
    }

    std::string commitHash;
    std::getline(branchFile, commitHash);

    branchFile.close();

    return commitHash;
}

Models::Commit loadCommit(const std::string& hash)
{
    Models::Commit commit;

    // Split hash into folder and filename
    std::string dirPrefix = hash.substr(0, 2);
    std::string fileSuffix = hash.substr(2);

    fs::path objectFile =
        fs::path(".aigit/objects") /
        dirPrefix /
        fileSuffix;

    // Open commit object
    std::ifstream inFile(objectFile, std::ios::binary);

    if (!inFile.is_open())
    {
        throw std::runtime_error("Failed to open commit object.");
    }

    // Read entire object
    std::stringstream buffer;
    buffer << inFile.rdbuf();

    std::string object = buffer.str();

    inFile.close();

    // Find end of header ("commit <size>\0")
    size_t nullPos = object.find('\0');

    if (nullPos == std::string::npos)
    {
        throw std::runtime_error("Invalid commit object.");
    }

    // Remove header
    std::string payload = object.substr(nullPos + 1);

    // Parse payload
    std::stringstream ss(payload);
    std::string line;

    // Read metadata
    while (std::getline(ss, line))
    {
        if (line.empty())
        {
            break;
        }

        if (line.substr(0, 5) == "tree ")
        {
            commit.treeHash = line.substr(5);
        }
        else if (line.substr(0, 7) == "parent ")
        {
            commit.parentHash = line.substr(7);
        }
        else if (line.substr(0, 7) == "author ")
        {
            commit.author = line.substr(7);
        }
        else if (line.substr(0, 10) == "committer ")
        {
            commit.committer = line.substr(10);
        }
    }

    // Read commit message
    std::string message;

    while (std::getline(ss, line))
    {
        message += line;

        if (!ss.eof())
        {
            message += '\n';
        }
    }

    commit.message = message;

    return commit;
}

void printCommit(const Models::Commit& commit)
{
    std::cout << "Commit" << std::endl;

    std::cout << "Tree: "
              << commit.treeHash
              << std::endl;

    if (!commit.parentHash.empty())
    {
        std::cout << "Parent: "
                  << commit.parentHash
                  << std::endl;
    }

    std::cout << "Author: "
              << commit.author
              << std::endl;

    std::cout << "Committer: "
              << commit.committer
              << std::endl;

    std::cout << "\nMessage:\n"
              << commit.message
              << std::endl;
}


}

namespace Commands
{

int runLog()
{
    try
    {
        // Find current branch
        std::string branchRef = getCurrentBranchRef();

        // Get latest commit on that branch
        std::string commitHash = getCurrentCommitHash(branchRef);

        // Traverse commit history
        while (!commitHash.empty())
        {
            Models::Commit commit = loadCommit(commitHash);

            printCommit(commit);

            commitHash = commit.parentHash;
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

}