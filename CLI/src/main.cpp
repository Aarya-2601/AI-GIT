#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include "commands/commands.hpp"
#include "commands/checkout.hpp"
#include "commands/branch.hpp"
#include "helpers/ui_theme.hpp"

namespace fs = std::filesystem;

static void printUsage()
{
    UI::initTerminal();
    std::cout << "\n";
    std::cout << UI::Color::CYAN << UI::Color::BOLD << "    🦎  AI-GIT " 
              << UI::Color::GREEN << "• "
              << UI::Color::TEAL << "Content-Addressable Storage VCS\n"
              << UI::Color::RESET;
    std::cout << UI::Color::DARK_SLATE << "    High-Performance FastCDC Chunking & Blake3 Cryptographic Storage\n" << UI::Color::RESET;
    std::cout << UI::Color::BORDER << "  ───────────────────────────────────────────────────────────────────\n" << UI::Color::RESET;

    std::cout << "  " << UI::Color::WHITE << UI::Color::BOLD << "USAGE:" << UI::Color::RESET << "\n";
    std::cout << "    " << UI::Color::CYAN << "ai-git " << UI::Color::GREEN << "<command> " << UI::Color::SLATE << "[<args>...]\n\n" << UI::Color::RESET;

    std::cout << "  " << UI::Color::WHITE << UI::Color::BOLD << "TERMINAL USER INTERFACE:" << UI::Color::RESET << "\n";
    std::cout << "    " << UI::Color::CYAN << UI::Color::BOLD << "tui          " 
              << UI::Color::RESET << "Launch the interactive Chameleon Terminal UI dashboard\n\n";

    std::cout << "  " << UI::Color::WHITE << UI::Color::BOLD << "REPOSITORY SETUP & WORKSPACE:" << UI::Color::RESET << "\n";
    std::cout << "    " << UI::Color::CYAN << "init         " << UI::Color::RESET << "Initialize a new AI-Git repository with FastCDC & Blake3 CAS\n";
    std::cout << "    " << UI::Color::CYAN << "status       " << UI::Color::RESET << "Show staged, modified, and untracked files with hash details\n";
    std::cout << "    " << UI::Color::CYAN << "hash-object  " << UI::Color::RESET << "Compute Blake3 CAS hash and inspect FastCDC chunk manifest\n\n";

    std::cout << "  " << UI::Color::WHITE << UI::Color::BOLD << "STAGING & HISTORY:" << UI::Color::RESET << "\n";
    std::cout << "    " << UI::Color::CYAN << "add          " << UI::Color::RESET << "Add file contents to the staging index (FastCDC chunked)\n";
    std::cout << "    " << UI::Color::CYAN << "commit       " << UI::Color::RESET << "Record staged changes into a new commit with tree manifest\n";
    std::cout << "    " << UI::Color::CYAN << "log          " << UI::Color::RESET << "Show commit history DAG with visual graph and metadata\n\n";

    std::cout << "  " << UI::Color::WHITE << UI::Color::BOLD << "BRANCHES & NAVIGATION:" << UI::Color::RESET << "\n";
    std::cout << "    " << UI::Color::CYAN << "branch       " << UI::Color::RESET << "List, create, or inspect branches\n";
    std::cout << "    " << UI::Color::CYAN << "checkout     " << UI::Color::RESET << "Switch branches and restore working tree\n\n";

    std::cout << "  " << UI::Color::WHITE << UI::Color::BOLD << "REMOTE & SYNC:" << UI::Color::RESET << "\n";
    std::cout << "    " << UI::Color::CYAN << "push         " << UI::Color::RESET << "Upload local objects and refs to remote server\n";
    std::cout << "    " << UI::Color::CYAN << "pull         " << UI::Color::RESET << "Download new objects and fast-forward branch\n";
    std::cout << "    " << UI::Color::CYAN << "clone        " << UI::Color::RESET << "Clone a remote repository\n\n";

    std::cout << "  " << UI::Color::WHITE << UI::Color::BOLD << "MAINTENANCE & INTEGRITY:" << UI::Color::RESET << "\n";
    std::cout << "    " << UI::Color::CYAN << "config       " << UI::Color::RESET << "Get and set repository options\n";
    std::cout << "    " << UI::Color::CYAN << "fsck         " << UI::Color::RESET << "Verify every object on disk and rebuild metadata.db\n";
    std::cout << "    " << UI::Color::CYAN << "migrate      " << UI::Color::RESET << "Rewrite legacy objects into the current on-disk format\n";
    std::cout << "    " << UI::Color::CYAN << "gc           " << UI::Color::RESET << "Delete unreferenced objects and pack storage\n";

    std::cout << UI::Color::BORDER << "  ───────────────────────────────────────────────────────────────────\n";
    std::cout << "  " << UI::Color::DARK_SLATE << "Try " << UI::Color::CYAN << "ai-git tui" << UI::Color::DARK_SLATE << " for the full interactive visual experience!\n" << UI::Color::RESET << std::endl;
}

int main(int argc, char* argv[]) {
    UI::initTerminal();
    std::vector<std::string> args(argv, argv + argc);

    if (args.size() < 2) {
        printUsage();
        return 1;
    }

    std::string command = args[1];

    if (command == "help" || command == "--help" || command == "-h") {
        printUsage();
        return 0;
    }

    if (command == "tui") {
        fs::path exeDir = fs::absolute(args[0]).parent_path();
        std::vector<fs::path> candidatePaths = {
            exeDir / "tui" / "aigit_tui.py",
            exeDir / "aigit_tui.py",
            exeDir.parent_path() / "tui" / "aigit_tui.py",
            exeDir.parent_path().parent_path() / "CLI" / "tui" / "aigit_tui.py",
            fs::current_path() / "CLI" / "tui" / "aigit_tui.py",
            fs::current_path() / "tui" / "aigit_tui.py",
            fs::current_path() / "aigit_tui.py"
        };
        fs::path tuiScript;
        for (const auto& p : candidatePaths) {
            if (fs::exists(p)) {
                tuiScript = p;
                break;
            }
        }
        if (tuiScript.empty()) {
            std::cerr << UI::Color::RED << "Error: AI-Git TUI engine (aigit_tui.py) not found." << UI::Color::RESET << std::endl;
            return 1;
        }
        std::string cmd = "python \"" + tuiScript.string() + "\"";
        return std::system(cmd.c_str());
    }

    if (command == "init") {
        return Commands::runInit();
    } 

    else if (command == "add") {
        std::vector<std::string> targets(args.begin() + 2, args.end());
        return Commands::runAdd(targets);
    }

    else if (command == "status") {
        return Commands::runStatus();
    }

    else if (command == "commit") {
        if (args.size() < 3) {
            std::cerr << UI::Color::RED << "Error: Commit message required." << UI::Color::RESET << std::endl;
            std::cerr << UI::Color::SLATE << "Usage: ai-git commit \"<message>\" or ai-git commit -m \"<message>\"" << UI::Color::RESET << std::endl;
            return 1;
        }

        std::string commitMessage;
        if (args[2] == "-m") {
            if (args.size() < 4) {
                std::cerr << UI::Color::RED << "Error: Missing commit message after -m flag." << UI::Color::RESET << std::endl;
                return 1;
            }
            commitMessage = args[3];
        } 
        else {
            commitMessage = args[2];
        }

        return Commands::runCommit(commitMessage);
    }

    else if (command == "hash-object") {
        if (args.size() < 3) {
            std::cerr << UI::Color::RED << "Error: 'hash-object' requires a valid filename parameter." << UI::Color::RESET << std::endl;
            std::cerr << UI::Color::SLATE << "Usage: ai-git hash-object <file> [--raw]" << UI::Color::RESET << std::endl;
            return 1;
        }

        bool verbose = true;
        std::string targetFile = args[2];
        if (targetFile == "--raw" || targetFile == "-q") {
            verbose = false;
            if (args.size() >= 4) targetFile = args[3];
        } else if (args.size() >= 4 && (args[3] == "--raw" || args[3] == "-q")) {
            verbose = false;
        }

        return Commands::runHashObject(targetFile, verbose);
    }
    
    else if (command == "log") {
        return Commands::runLog();
    }

    else if (command == "config") {
        std::vector<std::string> configArgs(args.begin() + 2, args.end());
        return Commands::runConfig(configArgs);
    }

    else if (command == "push") {
        std::string server = (args.size() >= 3) ? args[2] : "http://localhost:3000";
        return Commands::runPush(server);
    }

    else if (command == "pull") {
        std::string repoName = (args.size() >= 3) ? args[2] : "default-repo";
        return Commands::runPull(repoName);
    }
    
    else if (command == "clone") {
        if (args.size() < 3) {
            std::cerr << UI::Color::RED << "Error: 'clone' requires a repository name." << UI::Color::RESET << std::endl;
            std::cerr << UI::Color::SLATE << "Usage: ai-git clone <repo_name>" << UI::Color::RESET << std::endl;
            return 1;
        }
        std::string repoName = args[2];
        return Commands::runClone(repoName);
    }

    else if (command == "checkout") {
        if (args.size() < 3) {
            std::cerr << UI::Color::RED << "Error: 'checkout' requires a branch name." << UI::Color::RESET << std::endl;
            std::cerr << UI::Color::SLATE << "Usage: ai-git checkout <branch-name>" << UI::Color::RESET << std::endl;
            return 1;
        }
        return Commands::runCheckout(args[2]);
    }

    else if (command == "branch") {
        if (args.size() < 3) {
            Commands::listBranches();
            return 0;
        }
        Commands::runBranch(args);
        return 0;
    }

    else if (command == "fsck") {
        return Commands::runFsck();
    }

    else if (command == "migrate") {
        return Commands::runMigrate();
    }

    else if (command == "gc") {
        return Commands::runGc();
    }

    else {
        std::cerr << UI::Color::RED << "Error: Command '" << command << "' not recognized." << UI::Color::RESET << std::endl;
        std::cerr << UI::Color::SLATE << "Run 'ai-git --help' to see available commands." << UI::Color::RESET << std::endl;
        return 1;
    }
}