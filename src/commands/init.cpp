#include <iostream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
namespace Commands
{
int runInit()
{
    // Check if a repository already exists
    if (fs::exists(".aigit"))
    {
        std::cout << "Repository already exists.\n";
        return 1;
    }

    try
    {
        fs::create_directory(".aigit");
        fs::create_directory(".aigit/objects");
        fs::create_directories(".aigit/refs/heads"); //store pointers to branches and thier latest commit 

        std::ofstream head(".aigit/HEAD"); //pointer to the current branch, so initialized to main
        head << "ref: refs/heads/main\n";
        head.close();

        std::ofstream config(".aigit/config");
        config << "[core]\n";
        config << "\trepositoryformatversion = 0\n";
        config.close();

        std::ofstream index(".aigit/index"); // staging area, stores filees to be committed, so basically files on which git add is run
        index.close();

        std::cout << "Initialized empty AI-Git repository in "<< fs::absolute(".aigit") << std::endl;
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Initialization failed: "<< e.what() << std::endl;
    }

    return 0;
}
}