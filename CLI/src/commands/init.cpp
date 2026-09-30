#include "init.hpp"
#include "../core/filesystem.hpp"
#include "../storage/storage_manager.hpp"
#include "../helpers/ui_theme.hpp"
#include "../helpers/gitutils.hpp"
#include <iostream>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

namespace Commands
{
int runInit()
{
    UI::initTerminal();

    // Check if a repository already exists
    if (fs::exists(".aigit"))
    {
        std::cout << "\n"
                  << UI::Color::AMBER << "  ⚠️  Repository already initialized:\n"
                  << UI::Color::SLATE << "      " << fs::absolute(".aigit").string() << "\n"
                  << UI::Color::RESET << std::endl;
        return 1;
    }

    try
    {
        // 1. Create Core AI-Git directories
        fs::create_directory(".aigit");
        fs::create_directory(".aigit/objects");
        fs::create_directories(".aigit/refs/heads");

        // 2. HEAD ref pointing to default branch 'main'
        std::ofstream head(".aigit/HEAD");
        head << "ref: refs/heads/main\n";
        head.close();

        // 3. Initialize SQLite metadata database (CAS catalog)
        Storage::StorageManager storage(".aigit");
        storage.initialize();

        // 4. Default configuration
        std::ofstream config(".aigit/config");
        config << "[core]\n";
        config << "\trepositoryformatversion = 0\n";
        config << "\tchunkengine = fastcdc\n";
        config << "\thashalgorithm = blake3\n";
        config.close();

        // 5. Staging index
        std::ofstream index(".aigit/index");
        index.close();

        // Workspace directory name
        std::string currentDirName = fs::current_path().filename().string();
        if (currentDirName.empty()) currentDirName = "workspace";

        // Display Chameleon Header Banner
        std::cout << "\n";
        UI::printLogoBanner();
        std::cout << UI::Color::BORDER << "  ───────────────────────────────────────────────────────────────────\n" << UI::Color::RESET;

        std::cout << UI::Color::GREEN << UI::Color::BOLD << "  ✔  Initialized empty AI-Git repository\n" << UI::Color::RESET;
        std::cout << UI::Color::SLATE << "     Location: " << UI::Color::CYAN << fs::absolute(".aigit").string() << UI::Color::RESET << "\n\n";

        // Visual Tree Display of the Initialized Repository Structure
        std::cout << UI::Color::CYAN << UI::Color::BOLD << "  📂 " << currentDirName << "/\n" << UI::Color::RESET;

        // Highlight initialized .aigit internal structure in vibrant Chameleon Green & Cyan
        std::cout << UI::Color::BORDER << "  ├── " 
                  << UI::Color::GREEN << UI::Color::BOLD << "🦎 .aigit/                     " 
                  << UI::Color::BG_GREEN << " REPO ROOT " << UI::Color::RESET << "\n";

        std::cout << UI::Color::BORDER << "  │   ├── " 
                  << UI::Color::CYAN << "📄 HEAD                        " 
                  << UI::Color::SLATE << "-> " << UI::Color::TEAL << "ref: refs/heads/main\n" << UI::Color::RESET;

        std::cout << UI::Color::BORDER << "  │   ├── " 
                  << UI::Color::CYAN << "⚙️  config                      " 
                  << UI::Color::SLATE << "-> " << UI::Color::TEAL << "FastCDC (256KB-4MB) • Blake3 CAS\n" << UI::Color::RESET;

        std::cout << UI::Color::BORDER << "  │   ├── " 
                  << UI::Color::CYAN << "📋 index                       " 
                  << UI::Color::SLATE << "-> " << UI::Color::TEAL << "Staging Area\n" << UI::Color::RESET;

        std::cout << UI::Color::BORDER << "  │   ├── " 
                  << UI::Color::GREEN << UI::Color::BOLD << "🗄️  metadata.db                 " 
                  << UI::Color::SLATE << "-> " << UI::Color::BRIGHT_GREEN << "SQLite CAS Chunk Catalog\n" << UI::Color::RESET;

        std::cout << UI::Color::BORDER << "  │   ├── " 
                  << UI::Color::TEAL << "📦 objects/                    " 
                  << UI::Color::SLATE << "-> " << UI::Color::BLUE << "Content-Addressable Storage\n" << UI::Color::RESET;

        std::cout << UI::Color::BORDER << "  │   └── " 
                  << UI::Color::CYAN << "🌿 refs/heads/\n" << UI::Color::RESET;

        std::cout << UI::Color::BORDER << "  │       └── " 
                  << UI::Color::BLUE << "🌿 main                " 
                  << UI::Color::SLATE << "[default branch]\n" << UI::Color::RESET;

        // Show existing workspace files/directories in subtle slate to clearly show contrast
        int shownExisting = 0;
        for (const auto& item : fs::directory_iterator("."))
        {
            std::string name = item.path().filename().string();
            if (name == ".aigit") continue;
            if (Utils::isIgnoredPath(name)) continue;

            if (shownExisting == 0)
            {
                std::cout << UI::Color::BORDER << "  │\n" << UI::Color::RESET;
            }

            shownExisting++;
            if (shownExisting <= 6)
            {
                bool isDir = item.is_directory();
                std::string icon = isDir ? "📁 " : "📄 ";
                std::string suffix = isDir ? "/" : "";
                std::string sizeStr = "";
                if (!isDir)
                {
                    try {
                        sizeStr = " (" + UI::formatBytes(fs::file_size(item.path())) + ")";
                    } catch (...) {}
                }

                std::cout << UI::Color::BORDER << "  ├── " 
                          << UI::Color::SLATE << icon << name << suffix 
                          << UI::Color::DARK_SLATE << sizeStr << "\n" << UI::Color::RESET;
            }
        }
        if (shownExisting > 6)
        {
            std::cout << UI::Color::BORDER << "  ├── " 
                      << UI::Color::DARK_SLATE << "..." << (shownExisting - 6) << " more existing files\n" << UI::Color::RESET;
        }

        std::cout << UI::Color::BORDER << "  └── " << UI::Color::DARK_SLATE << "(end of workspace tree)\n" << UI::Color::RESET;

        // Details card
        std::cout << "\n" << UI::Color::BORDER << "  ╭──────────────────────────────────────────────────────────────────╮\n";
        std::cout << "  │ " << UI::Color::CYAN << UI::Color::BOLD << "Repository Settings:" << UI::Color::RESET << "                                            │\n";
        std::cout << "  │   " << UI::Color::SLATE << "• Default Branch  : " << UI::Color::CYAN << "main" << UI::Color::RESET << "                                           │\n";
        std::cout << "  │   " << UI::Color::SLATE << "• Storage Engine  : " << UI::Color::GREEN << "FastCDC (Min: 256KB, Avg: 1MB, Max: 4MB)" << UI::Color::RESET << "    │\n";
        std::cout << "  │   " << UI::Color::SLATE << "• Hash Algorithm  : " << UI::Color::TEAL << "Blake3 (256-bit crypto CAS)" << UI::Color::RESET << "                 │\n";
        std::cout << "  │   " << UI::Color::SLATE << "• Next Steps      : " << UI::Color::WHITE << "ai-git status" << UI::Color::SLATE << " or " << UI::Color::CYAN << "ai-git tui" << UI::Color::RESET << "                     │\n";
        std::cout << UI::Color::BORDER << "  ╰──────────────────────────────────────────────────────────────────╯\n\n" << UI::Color::RESET;
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << UI::Color::RED << "  Initialization failed: " << e.what() << UI::Color::RESET << std::endl;
        return 1;
    }

    return 0;
}
}