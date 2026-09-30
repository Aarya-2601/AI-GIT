#include "log.hpp"
#include "../core/object_io.hpp"
#include "../helpers/gitutils.hpp"
#include "../helpers/ui_theme.hpp"
#include <iostream>
#include <chrono>
#include <ctime>

namespace fs = std::filesystem;
using namespace std;

namespace
{
void printCommitCard(const Models::Commit& commit, const std::string& commitHash, bool isHead, const std::string& branchName)
{
    // Node marker: * commit <hash> (HEAD -> main)
    std::cout << "  " << UI::Color::CYAN << UI::Color::BOLD << "* " << UI::Color::RESET
              << UI::Color::SLATE << "commit " 
              << UI::Color::GREEN << UI::Color::BOLD << commitHash << UI::Color::RESET;

    if (isHead)
    {
        std::cout << " " << UI::Color::CYAN << UI::Color::BOLD << "(HEAD -> " 
                  << UI::Color::WHITE << branchName << UI::Color::CYAN << ")" << UI::Color::RESET;
    }
    std::cout << "\n";

    // Connector line
    std::cout << "  " << UI::Color::BORDER << "│ " << UI::Color::RESET;

    const auto& author = commit.getAuthor();
    std::cout << UI::Color::SLATE << "Author:    " << UI::Color::WHITE << author.name;
    if (!author.email.empty())
    {
        std::cout << UI::Color::DARK_SLATE << " <" << author.email << ">";
    }
    std::cout << UI::Color::RESET << "\n";

    std::cout << "  " << UI::Color::BORDER << "│ " << UI::Color::RESET;
    std::cout << UI::Color::SLATE << "Tree:      " << UI::Color::BLUE << commit.getTreeHash() << UI::Color::RESET << "\n";

    const auto& parents = commit.getParentHashes();
    for (const auto& parent : parents)
    {
        if (!parent.empty())
        {
            std::cout << "  " << UI::Color::BORDER << "│ " << UI::Color::RESET;
            std::cout << UI::Color::SLATE << "Parent:    " << UI::Color::DARK_SLATE << parent.substr(0, 16) << "…" << UI::Color::RESET << "\n";
        }
    }

    std::cout << "  " << UI::Color::BORDER << "│\n" << UI::Color::RESET;
    std::cout << "  " << UI::Color::BORDER << "│     " 
              << UI::Color::WHITE << UI::Color::BOLD << commit.getMessage() 
              << UI::Color::RESET << "\n";
    std::cout << "  " << UI::Color::BORDER << "│\n" << UI::Color::RESET;
}
}

namespace Commands
{
int runLog()
{
    UI::initTerminal();

    try
    {   
        if (!fs::exists(".aigit"))
        {
            std::cerr << "\n" << UI::Color::RED << "  fatal: not an ai-git repository (or any of the parent directories): .aigit" << UI::Color::RESET << "\n\n";
            return 1;
        }

        std::string commitHash = Utils::getCurrentCommitHash();
        if (commitHash.empty())
        {
            std::cout << "\n" << UI::Color::AMBER << "  No commits yet on current branch." << UI::Color::RESET << "\n";
            std::cout << UI::Color::SLATE << "  Run \"ai-git add <files>\" and \"ai-git commit -m <msg>\" to create the first commit.\n\n" << UI::Color::RESET;
            return 0;
        }

        std::string branchName = Utils::getCurrentBranchName();
        if (branchName.empty()) branchName = "main";

        std::cout << "\n" << UI::Color::CYAN << UI::Color::BOLD << "  🦎 AI-GIT COMMIT HISTORY DAG " 
                  << UI::Color::BORDER << "• "
                  << UI::Color::SLATE << "Branch: " << UI::Color::CYAN << " " << branchName << UI::Color::RESET << "\n";
        std::cout << UI::Color::BORDER << "  ───────────────────────────────────────────────────────────────────\n" << UI::Color::RESET;

        bool isHead = true;
        int count = 0;

        // Traverse commit history
        while (!commitHash.empty())
        {
            Models::Commit commit = Core::loadCommit(commitHash);
            printCommitCard(commit, commitHash, isHead, branchName);
            isHead = false;
            count++;

            const auto& parents = commit.getParentHashes();
            if (!parents.empty())
            {
                commitHash = parents[0];
            }
            else
            {
                commitHash = "";
            }
        }

        std::cout << "  " << UI::Color::GREEN << "● " 
                  << UI::Color::DARK_SLATE << "(root commit reached — " << count << " commit" << (count == 1 ? "" : "s") << " total)\n\n" << UI::Color::RESET;

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << UI::Color::RED << "  Log Error: " << e.what() << UI::Color::RESET << std::endl;
        return 1;
    }
}
}