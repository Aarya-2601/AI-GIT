#include "commands/commit.hpp"

namespace fs=std::filesystem;
using namespace std;

namespace
{

std::vector<Core::IndexEntry> readIndex()
{   
    Core::Index index;
    index.load(".aigit/index");
    std::vector<Core::IndexEntry> entries;

    for (const auto& [path, entry] : index.getEntries())
    {
        entries.push_back(entry);
    }   
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

std::string writeTree(Models::TreeNode* node)
{   
    Models::Tree treeObj;
    for (auto& child : node->children)
    {
        if (child.second->isDirectory)
        {   
            child.second->hash = writeTree(child.second.get());

            Models::TreeDef def;
            def.mode = "040000";
            def.name = child.second->name;
            def.hash = child.second->hash;
            def.isSubtree = true;

            treeObj.addEntry(def);
        }
        else{
            Models::TreeDef def;
            def.mode = "100644";
            def.name = child.second->name;
            def.hash = child.second->hash;
            def.isSubtree = false;

            treeObj.addEntry(def);
        }
    }

    std::string payload = treeObj.serialize();
    std::string treeHash = Core::calcSHA256(payload);

    if (treeHash.empty())
    {
        throw std::runtime_error("Failed to hash tree object.");
    }

    std::string compressedObject = Core::compressString(payload);

    if (!Core::Storage::writeObject(treeHash, compressedObject))
    {
        throw std::runtime_error("Failed to store tree object.");
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

    Core::Config config;
    config.load(".aigit/config");

    long long timestamp = static_cast<long long>(std::time(nullptr));
    std::string timezone = "+0000";

    Models::CommitMsg author{
        config.getAuthorName(),
        config.getAuthorEmail(),
        timestamp,
        timezone
    };
    Models::CommitMsg committer = author;

    std::vector<std::string> parents;
    if (!parentHash.empty())
    {
        parents.push_back(parentHash);
    }

    Models::Commit commitObj(
        rootTreeHash,
        parents,
        author,
        committer,
        message,
        timestamp,
        timezone
    );

    std::string uncompressedObject = commitObj.serialize();

    std::string commitHash = Core::calcSHA256(uncompressedObject);

    if (commitHash.empty())
    {
        throw std::runtime_error("Failed to hash commit object.");
    }

    std::string compressedObject = Core::compressString(uncompressedObject);

    if (!Core::Storage::writeObject(commitHash, compressedObject))
    {
        throw std::runtime_error("Failed to store commit object.");
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

    while (!refLine.empty() && (refLine.back() == '\r' || refLine.back() == '\n' || refLine.back() == ' '))
    {
        refLine.pop_back();
    }
    
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