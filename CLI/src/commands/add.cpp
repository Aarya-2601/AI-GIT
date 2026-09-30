#include "add.hpp"
#include "../core/filesystem.hpp"
#include "../core/hashing.hpp"
#include "../storage/storage_manager.hpp"
#include "../helpers/gitutils.hpp"
#include "../helpers/ui_theme.hpp"
#include <iostream>
#include <iomanip>

namespace fs = std::filesystem;
using namespace std;

namespace Commands
{
    static bool processfile(const fs::path& filePath, Core::Index& indexEntries, int& addedCount)
    {
        std::string normPath = Utils::normalizePath(filePath);

        if (Utils::isIgnoredPath(normPath))
        {
            return true;
        }

        try {
            uintmax_t fileSize = 0;
            try {
                if (fs::exists(filePath)) fileSize = fs::file_size(filePath);
            } catch (...) {}

            // StorageManager automatically handles FastCDC chunking for files >256KB
            // and blob storage for files <=256KB, returning the manifest/blob object ID
            Storage::StorageManager storageManager(".aigit");
            std::string objectId = storageManager.storeFile(filePath);

            if (objectId.empty()) 
            {
                std::cerr << UI::Color::RED << "  Error: Storage manager failed to store object for: " << normPath << UI::Color::RESET << std::endl;
                return false;
            }

            const auto& existingEntries = indexEntries.getEntries();
            auto it = existingEntries.find(normPath);
            if (it != existingEntries.end() && it->second.hash == objectId)
            {
                std::cout << "  " << UI::Color::DARK_SLATE << "• " 
                          << std::left << std::setw(32) << normPath 
                          << " " << std::right << std::setw(10) << UI::formatBytes(fileSize)
                          << "  " << UI::Color::SLATE << "hash: " << objectId.substr(0, 10) << "… "
                          << UI::Color::DARK_SLATE << "[unchanged]\n" << UI::Color::RESET;
                return true;
            }

            indexEntries.addEntry(Core::IndexEntry(normPath, objectId, "100644"));
            addedCount++;

            std::string storageMode = (fileSize > 256 * 1024) ? "FastCDC" : "Blob";

            std::cout << "  " << UI::Color::GREEN << UI::Color::BOLD << "✔ " << UI::Color::RESET
                      << UI::Color::WHITE << UI::Color::BOLD << std::left << std::setw(32) << normPath << UI::Color::RESET
                      << " " << UI::Color::TEAL << std::right << std::setw(10) << UI::formatBytes(fileSize) << "  " << UI::Color::RESET
                      << UI::Color::SLATE << "hash: " 
                      << UI::Color::CYAN << objectId.substr(0, 10) << "… " << UI::Color::RESET
                      << UI::Color::BORDER << "[" << UI::Color::GREEN << storageMode << UI::Color::BORDER << "]\n" << UI::Color::RESET;

            return true;
        }
        catch(const std::exception& e) {
            std::cerr << UI::Color::RED << "  Storage Error on " << normPath << ": " << e.what() << UI::Color::RESET << std::endl;
            return false;
        }
    }

    int runAdd(const std::vector<std::string>& targets)
    {
        UI::initTerminal();

        if (!fs::exists(".aigit"))
        {
            std::cerr << "\n" << UI::Color::RED << "  Error: Not an AI-Git repository." << UI::Color::RESET << "\n\n";
            return 1;
        }
        if (targets.empty())
        {
            std::cerr << UI::Color::SLATE << "  Nothing specified, nothing added." << UI::Color::RESET << std::endl;
            return 0;
        }
        
        Core::Index indexEntries;
        indexEntries.load(".aigit/index");
        
        int addedCount = 0;

        std::cout << "\n" << UI::Color::CYAN << UI::Color::BOLD << "  🦎 AI-GIT STAGING" << UI::Color::RESET << "\n";
        std::cout << UI::Color::BORDER << "  ───────────────────────────────────────────────────────────────────\n" << UI::Color::RESET;

        for (const auto& target : targets)
        {
            fs::path targetPath(target);
            if (!fs::exists(targetPath)) {
                std::cerr << UI::Color::RED << "  Error: Path does not exist: " << target << UI::Color::RESET << std::endl;
                continue;
            }

            if (fs::is_directory(targetPath))
            {
                for (const auto& entry : fs::recursive_directory_iterator(targetPath)) {
                    std::string pStr = Utils::normalizePath(entry.path());

                    if (Utils::isIgnoredPath(pStr)) {
                        continue;
                    }

                    if (fs::is_regular_file(entry.status()))
                    {
                        processfile(entry.path(), indexEntries, addedCount);
                    }
                }
            }
            else if (fs::is_regular_file(targetPath))
            {
                processfile(targetPath, indexEntries, addedCount);
            }
            else {
                std::cerr << UI::Color::AMBER << "  Warning: Skipping unsupported path: " << target << UI::Color::RESET << std::endl;
            }
        }

        std::ofstream indexOut(".aigit/index", std::ios::trunc);
        if (!indexOut.is_open())
        {
            std::cerr << UI::Color::RED << "  Error: Could not open index for writing." << UI::Color::RESET << std::endl;
            return 1;
        }
        indexEntries.save(".aigit/index");

        std::cout << UI::Color::BORDER << "  ───────────────────────────────────────────────────────────────────\n";
        if (addedCount > 0)
        {
            std::cout << "  " << UI::Color::GREEN << UI::Color::BOLD << "Staged " << addedCount << " file(s) into index." 
                      << UI::Color::SLATE << " Run " << UI::Color::CYAN << "ai-git commit -m \"<message>\"" << UI::Color::SLATE << " to commit.\n";
        }
        else
        {
            std::cout << "  " << UI::Color::SLATE << "Index up to date. No new changes staged.\n";
        }
        std::cout << UI::Color::RESET << std::endl;

        return 0;
    }

    int runAdd(const std::string& filePath)
    {
        return runAdd(std::vector<std::string>{filePath});
    }
}