#include "status.hpp"
#include "../core/object_io.hpp"
#include "../helpers/gitutils.hpp"
#include "../helpers/ui_theme.hpp"
#include "../storage/storage_manager.hpp"
#include <iostream>
#include <iomanip>
#include <set>

namespace fs = std::filesystem;

namespace Commands
{
    static void collectHeadEntries(const std::string& treeHash, const std::string& currentPrefix, std::unordered_map<std::string, std::string>& headEntries)
    {
        if (treeHash.empty()) return;

        Models::Tree tree;
        try {
            tree = Core::loadTree(treeHash);
        } catch (const std::exception&) {
            return;
        }

        for (const auto& entry : tree.getEntries())
        {
            std::string fullPath = currentPrefix.empty() ? entry.name : currentPrefix + "/" + entry.name;

            if (entry.isSubtree)
            {
                collectHeadEntries(entry.hash, fullPath, headEntries);
            }
            else
            {
                headEntries[fullPath] = entry.hash;
            }
        }
    }

    static std::unordered_map<std::string, std::string> getHeadCommitEntries()
    {
        std::unordered_map<std::string, std::string> headEntries;

        std::string commitHash = Utils::getCurrentCommitHash();
        if (commitHash.empty()) return headEntries;

        try {
            Models::Commit commit = Core::loadCommit(commitHash);
            collectHeadEntries(commit.getTreeHash(), "", headEntries);
        } catch (const std::exception&) {
            return headEntries;
        }

        return headEntries;
    }

    int runStatus()
    {
        UI::initTerminal();

        if (!fs::exists(".aigit"))
        {
            std::cerr << "\n" << UI::Color::RED << "  Error: Not an AI-Git repository (or any of the parent directories)." << UI::Color::RESET << "\n\n";
            return 1;
        }

        Core::Index index;
        index.load(".aigit/index");

        auto headEntries = getHeadCommitEntries();
        std::string currentBranch = Utils::getCurrentBranchName();
        if (currentBranch.empty()) currentBranch = "main";

        std::string currentCommit = Utils::getCurrentCommitHash();

        std::vector<std::tuple<std::string, std::string, std::string>> stagedFiles; // <label, path, hash>
        std::vector<std::string> modifiedFiles;
        std::vector<std::string> deletedFiles;
        std::vector<std::string> untrackedFiles;
        std::set<std::string> seenDiskFiles;

        for (const auto& [path, entry] : index.getEntries())
        {
            auto headIt = headEntries.find(path);
            if (headIt == headEntries.end()) {
                stagedFiles.push_back({"NEW", path, entry.hash});
            } else if (headIt->second != entry.hash) {
                stagedFiles.push_back({"MODIFIED", path, entry.hash});
            }
        }

        for (const auto& entry : fs::recursive_directory_iterator("."))
        {      
            if (!entry.is_regular_file()) continue;

            std::string pStr = Utils::normalizePath(entry.path());

            if (Utils::isIgnoredPath(pStr))
            {
                continue;
            }

            std::string filePath = pStr;
            seenDiskFiles.insert(filePath);

            const auto& indexMap = index.getEntries();
            auto idxIt = indexMap.find(filePath);

            if (idxIt == indexMap.end()) 
            {
                untrackedFiles.push_back(filePath);
            }
            else
            {
                try {
                    Storage::StorageManager storageManager(".aigit");
                    std::string objectId = storageManager.computeObjectId(entry.path());

                    if (objectId != idxIt->second.hash)
                    {
                        modifiedFiles.push_back(filePath);
                    }
                }
                catch (const std::exception&) {
                    std::cerr << UI::Color::RED << "  Error: Failed to open file for reading: " << filePath << UI::Color::RESET << std::endl;
                }
            }
        }

        for (const auto& [path, entry] : index.getEntries())
        {
            if (seenDiskFiles.find(path) == seenDiskFiles.end())
            {
                deletedFiles.push_back(path);
            }
        }

        // --- Render Chameleon Status Dashboard ---
        std::cout << "\n";
        std::cout << UI::Color::CYAN << UI::Color::BOLD << "  🦎 AI-GIT STATUS " << UI::Color::RESET
                  << UI::Color::BORDER << "• "
                  << UI::Color::SLATE << "Branch: " << UI::Color::CYAN << UI::Color::BOLD << " " << currentBranch << UI::Color::RESET;
        
        if (!currentCommit.empty())
        {
            std::cout << UI::Color::BORDER << " • " 
                      << UI::Color::SLATE << "HEAD: " 
                      << UI::Color::GREEN << currentCommit.substr(0, 10) << UI::Color::RESET;
        }
        else
        {
            std::cout << UI::Color::BORDER << " • " 
                      << UI::Color::DARK_SLATE << "HEAD: (initial commit)" << UI::Color::RESET;
        }
        std::cout << "\n" << UI::Color::BORDER << "  ───────────────────────────────────────────────────────────────────\n" << UI::Color::RESET;

        bool hasChanges = false;

        // 1. Staged Files Section (Vibrant Green)
        if (!stagedFiles.empty())
        {
            hasChanges = true;
            std::cout << UI::Color::GREEN << UI::Color::BOLD << "  Changes to be committed:" << UI::Color::RESET << "\n";
            std::cout << UI::Color::DARK_SLATE << "  (use \"ai-git commit -m <msg>\" to commit staged changes)\n\n" << UI::Color::RESET;

            for (const auto& [label, file, hash] : stagedFiles)
            {
                std::string badge = (label == "NEW") ? "[+ NEW]     " : "[~ MODIFIED]";
                std::string sizeStr = "-";
                try {
                    if (fs::exists(file)) sizeStr = UI::formatBytes(fs::file_size(file));
                } catch (...) {}

                std::cout << "    " << UI::Color::GREEN << UI::Color::BOLD << badge << " " 
                          << UI::Color::WHITE << UI::Color::BOLD << std::left << std::setw(28) << file << UI::Color::RESET
                          << UI::Color::TEAL << std::right << std::setw(10) << sizeStr << "  " << UI::Color::RESET
                          << UI::Color::SLATE << "hash: " 
                          << UI::Color::CYAN << hash.substr(0, 12) << "…" << UI::Color::RESET << "\n";
            }
            std::cout << "\n";
        }

        // 2. Unstaged Modifications & Deletions (Amber / Red)
        if (!modifiedFiles.empty() || !deletedFiles.empty())
        {
            hasChanges = true;
            std::cout << UI::Color::AMBER << UI::Color::BOLD << "  Changes not staged for commit:" << UI::Color::RESET << "\n";
            std::cout << UI::Color::DARK_SLATE << "  (use \"ai-git add <file>...\" to update what will be committed)\n\n" << UI::Color::RESET;

            for (const auto& file : modifiedFiles)
            {
                std::string sizeStr = "-";
                try {
                    if (fs::exists(file)) sizeStr = UI::formatBytes(fs::file_size(file));
                } catch (...) {}

                std::cout << "    " << UI::Color::AMBER << UI::Color::BOLD << "[~ MODIFIED] " 
                          << UI::Color::WHITE << std::left << std::setw(28) << file << UI::Color::RESET
                          << UI::Color::SLATE << std::right << std::setw(10) << sizeStr << "  "
                          << UI::Color::DARK_SLATE << "(working tree modified)\n" << UI::Color::RESET;
            }
            for (const auto& file : deletedFiles)
            {
                std::cout << "    " << UI::Color::RED << UI::Color::BOLD << "[- DELETED]  " 
                          << UI::Color::WHITE << std::left << std::setw(28) << file << UI::Color::RESET
                          << UI::Color::SLATE << std::right << std::setw(10) << "0 B" << "  "
                          << UI::Color::RED << "(deleted from disk)\n" << UI::Color::RESET;
            }
            std::cout << "\n";
        }

        // 3. Untracked Files (Slate / Cyan)
        if (!untrackedFiles.empty())
        {
            hasChanges = true;
            std::cout << UI::Color::BLUE << UI::Color::BOLD << "  Untracked files:" << UI::Color::RESET << "\n";
            std::cout << UI::Color::DARK_SLATE << "  (use \"ai-git add <file>...\" to begin tracking and FastCDC chunking)\n\n" << UI::Color::RESET;

            for (const auto& file : untrackedFiles)
            {
                std::string sizeStr = "-";
                try {
                    if (fs::exists(file)) sizeStr = UI::formatBytes(fs::file_size(file));
                } catch (...) {}

                std::cout << "    " << UI::Color::SLATE << "[? UNTRACKED] " 
                          << UI::Color::WHITE << std::left << std::setw(27) << file << UI::Color::RESET
                          << UI::Color::DARK_SLATE << std::right << std::setw(10) << sizeStr << "\n" << UI::Color::RESET;
            }
            std::cout << "\n";
        }

        // Summary Line
        if (!hasChanges)
        {
            std::cout << UI::Color::GREEN << UI::Color::BOLD << "  ✔ Working tree clean "
                      << UI::Color::SLATE << "— nothing to commit, no staged or modified files.\n" << UI::Color::RESET;
        }

        // Interactive shortcuts bar
        std::cout << UI::Color::BORDER << "  ───────────────────────────────────────────────────────────────────\n";
        std::cout << "  " << UI::Color::DARK_SLATE << "Shortcuts: "
                  << UI::Color::CYAN << "ai-git add <file>   "
                  << UI::Color::GREEN << "ai-git commit -m \"msg\"   "
                  << UI::Color::TEAL << "ai-git tui (Interactive UI)\n" << UI::Color::RESET << std::endl;

        return 0;
    }
}