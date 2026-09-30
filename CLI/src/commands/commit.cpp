#include "../models/object.hpp"
#include "../core/index.hpp"
#include "../core/config.hpp"
#include "../models/tree.hpp"
#include "../models/commit.hpp"
#include "../core/hashing.hpp"
#include "../core/filesystem.hpp"
#include "../helpers/gitutils.hpp"
#include "../helpers/ui_theme.hpp"
#include "../storage/storage_manager.hpp"
#include "../commands/commit.hpp"
using namespace std;
#include <filesystem>
namespace fs=std::filesystem;
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
    auto root =std::make_unique<Models::TreeNode>("", true);

    for (const auto& entry : entries)
    {
        fs::path currentPath(entry.path);
        Models::TreeNode* current = root.get();

        for (auto it = currentPath.begin();
             it != currentPath.end();
             ++it)
        {
            std::string name = it->string();
            bool isLast =(std::next(it) == currentPath.end());
            auto child =current->children.find(name);

            if (child == current->children.end())
            {
                current->children[name] =std::make_unique<Models::TreeNode>(name,!isLast);

                child =current->children.find(name);
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
        else
        {
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

    // ID stays SHA-256 of the full serialized payload (including its
    // "tree <n>\0" wrapper) exactly as before -- only the storage backend
    // changes here, so existing tree hashes are unaffected.
    Storage::StorageManager storageManager(".aigit");
    storageManager.storeObject(treeHash, payload, "tree");

    node->hash = treeHash;

    return treeHash;
}

std::string writeCommit(const std::string& rootTreeHash,const std::string& message)
{
    std::string parentHash = Utils::getCurrentCommitHash();

    Core::Config config;
    config.load(".aigit/config");

    long long timestamp = static_cast<long long>(std::time(nullptr));
    std::string timezone = "+0000";

    Models::CommitMsg author
    {
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

    // Same ID scheme as before (SHA-256 of the wrapper+body payload);
    // only the storage backend changes.
    Storage::StorageManager storageManager(".aigit");
    storageManager.storeObject(commitHash, uncompressedObject, "commit");

    return commitHash;
}

void updateHEAD(const std::string& commitHash)
{
    std::string branchName = Utils::getCurrentBranchName();

    if (branchName.empty())
    {
        throw std::runtime_error("Invalid HEAD format.");
    }

    Utils::writeBranchRef(branchName, commitHash);
}

}

namespace Commands
{

int runCommit(const std::string& message)
{
    UI::initTerminal();

    try
    {
        auto entries = readIndex();

        if (entries.empty())
        {
            std::cout << "\n" << UI::Color::AMBER << "  ⚠️  Nothing to commit, working tree clean or no changes staged." << UI::Color::RESET << "\n";
            std::cout << UI::Color::SLATE << "      (use \"ai-git add <file>...\" to stage files first)\n\n" << UI::Color::RESET;
            return 1;
        }

        auto root = buildDirectoryTree(entries);

        std::string rootTreeHash = writeTree(root.get());
        std::string commitHash = writeCommit(rootTreeHash, message);

        updateHEAD(commitHash);

        std::string currentBranch = Utils::getCurrentBranchName();
        if (currentBranch.empty()) currentBranch = "main";

        // Display Chameleon Commit Card
        std::cout << "\n";
        std::cout << UI::Color::BORDER << "  ╭──────────────────────────────────────────────────────────────────╮\n";
        std::cout << "  │ " << UI::Color::GREEN << UI::Color::BOLD << "🦎 AI-GIT COMMIT CREATED" << UI::Color::RESET 
                  << "  [" << UI::Color::CYAN << " " << currentBranch << UI::Color::RESET << " " 
                  << UI::Color::GREEN << commitHash.substr(0, 7) << UI::Color::BORDER << "]                                  │\n";
        std::cout << "  ├──────────────────────────────────────────────────────────────────┤\n";
        std::cout << "  │ " << UI::Color::SLATE << "Commit ID : " << UI::Color::GREEN << commitHash << UI::Color::BORDER << " │\n";
        std::cout << "  │ " << UI::Color::SLATE << "Tree ID   : " << UI::Color::BLUE << rootTreeHash << UI::Color::BORDER << " │\n";
        std::cout << "  │ " << UI::Color::SLATE << "Message   : " << UI::Color::WHITE << UI::Color::BOLD << "\"" << message << "\"" << UI::Color::RESET << "\n";
        std::cout << "  │ " << UI::Color::SLATE << "Entries   : " << UI::Color::TEAL << entries.size() << " files recorded in commit manifest" << UI::Color::RESET << "\n";
        std::cout << UI::Color::BORDER << "  ╰──────────────────────────────────────────────────────────────────╯\n\n" << UI::Color::RESET;

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << UI::Color::RED << "  Commit Error: " << e.what() << UI::Color::RESET << std::endl;
        return 1;
    }
}
}