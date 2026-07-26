#include <iostream>
#include <fstream>
#include <sstream>

#include "../commands/commit.hpp"
#include "../core/index.hpp"
#include "../models/tree.hpp"
#include "../core/hashing.hpp"
#include "core/filesystem.hpp"

using namespace std;

namespace
{

std::vector<Core::IndexEntry> readIndex()
{
    std::vector<Core::IndexEntry> entries;

    std::ifstream index(".aigit/index");

    if (!index.is_open())
    {
        throw std::runtime_error("Failed to open index.");
    }

    std::string hash;
    std::string path;

    while (index >> hash >> path)
    {
        Core::IndexEntry entry;

        entry.path = path;
        entry.hash = hash;

        entries.push_back(entry);
    }

    index.close();

    return entries;
}

std::unique_ptr<Models::TreeNode>
buildDirectoryTree(const std::vector<Core::IndexEntry>& entries)
{
    auto root =
        std::make_unique<Models::TreeNode>("", true);

    for (const auto& entry : entries)
    {
        fs::path currentPath(entry.path);

        Models::TreeNode* current = root.get();

        for (auto it = currentPath.begin();
             it != currentPath.end();
             ++it)
        {
            std::string name = it->string();

            bool isLast =
                (std::next(it) == currentPath.end());

            auto child =
                current->children.find(name);

            if (child == current->children.end())
            {
                current->children[name] =
                    std::make_unique<Models::TreeNode>(
                        name,
                        !isLast);

                child =
                    current->children.find(name);
            }

            if (isLast)
            {
                child->second->hash = entry.hash;
            }

            current = child->second.get();
        }
    }

    return root;
}

std::string createTreeObject(Models::TreeNode* node)
{
    std::ostringstream payload;

    for (const auto& child : node->children)
    {
        if (child.second->isDirectory)
        {
            payload
                << "40000 "
                << child.second->name
                << " "
                << child.second->hash
                << '\n';
        }
        else
        {
            payload
                << "100644 "
                << child.second->name
                << " "
                << child.second->hash
                << '\n';
        }
    }

    return payload.str();
}

std::string writeTree(Models::TreeNode* node)
{
    for (auto& child : node->children)
    {
        if (child.second->isDirectory)
        {
            child.second->hash = writeTree(child.second.get());
        }
    }

    std::string payload = createTreeObject(node);

    std::string header =
        "tree " +
        std::to_string(payload.size()) +
        '\0';

    std::string object = header + payload;

    std::string treeHash = Core::calcSHA256(object);

    if (treeHash.empty())
    {
        throw std::runtime_error("Failed to hash tree object.");
    }

    std::string dirPrefix = treeHash.substr(0, 2);
    std::string fileSuffix = treeHash.substr(2);

    fs::path objectsRoot = ".aigit/objects";
    fs::path objectFolder = objectsRoot / dirPrefix;
    fs::path objectFile = objectFolder / fileSuffix;

    fs::create_directories(objectFolder);

    if (!fs::exists(objectFile))
    {
        std::ofstream outFile(objectFile, std::ios::binary);

        if (!outFile.is_open())
        {
            throw std::runtime_error("Failed to store tree object.");
        }

        outFile << object;
        outFile.close();
    }

    node->hash = treeHash;

    return treeHash;
}

std::string writeCommit(const std::string& rootTreeHash,
                        const std::string& message)
{
    std::string parentHash;

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
        throw std::runtime_error("Invalid HEAD.");
    }

    std::string refPath = refLine.substr(5);

    std::ifstream branchFile(".aigit/" + refPath);

    if (branchFile.is_open())
    {
        std::getline(branchFile, parentHash);
        branchFile.close();
    }

    std::ostringstream payload;

    payload << "tree "
            << rootTreeHash
            << '\n';

    if (!parentHash.empty())
    {
        payload << "parent "
                << parentHash
                << '\n';
    }

    payload << "author AI-Git\n";
    payload << "committer AI-Git\n";
    payload << '\n';
    payload << message << '\n';

    std::string payloadString = payload.str();

    std::string header =
        "commit " +
        std::to_string(payloadString.size()) +
        '\0';

    std::string object =
        header + payloadString;

            std::string commitHash = Core::calcSHA256(object);

    if (commitHash.empty())
    {
        throw std::runtime_error("Failed to hash commit object.");
    }

    std::string dirPrefix = commitHash.substr(0, 2);
    std::string fileSuffix = commitHash.substr(2);

    fs::path objectsRoot = ".aigit/objects";
    fs::path objectFolder = objectsRoot / dirPrefix;
    fs::path objectFile = objectFolder / fileSuffix;

    fs::create_directories(objectFolder);

    if (!fs::exists(objectFile))
    {
        std::ofstream outFile(objectFile, std::ios::binary);

        if (!outFile.is_open())
        {
            throw std::runtime_error("Failed to store commit object.");
        }

        outFile << object;
        outFile.close();
    }

    return commitHash;
}

void updateHEAD(const std::string& commitHash)
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

    std::string refPath = refLine.substr(5);

    std::ofstream branchFile(".aigit/" + refPath,
                             std::ios::trunc);

    if (!branchFile.is_open())
    {
        throw std::runtime_error("Failed to update branch.");
    }

    branchFile << commitHash;

    branchFile.close();
}

}

namespace Commands
{

int runCommit(const std::string& message)
{
    try
    {
        auto entries = readIndex();

        if (entries.empty())
        {
            std::cerr << "Nothing to commit." << std::endl;
            return 1;
        }

        auto root = buildDirectoryTree(entries);

        std::string rootTreeHash =
            writeTree(root.get());

        std::string commitHash =
            writeCommit(rootTreeHash, message);

        updateHEAD(commitHash);

        std::cout
            << "Commit created successfully."
            << std::endl;

        std::cout
            << "Commit: "
            << commitHash
            << std::endl;

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr
            << e.what()
            << std::endl;

        return 1;
    }
}

}
