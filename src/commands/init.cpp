#include <iostream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

void init()
{
    // Check if a repository already exists
    if (fs::exists(".aigit"))
    {
        std::cout << "Repository already exists.\n";
        return;
    }

    try
    {
        fs::create_directory(".aigit");
        fs::create_directory(".aigit/objects");
        fs::create_directories(".aigit/refs/heads");

        std::ofstream head(".aigit/HEAD");
        head << "ref: refs/heads/main\n";
        head.close();

        std::ofstream config(".aigit/config");
        config << "[core]\n";
        config << "\trepositoryformatversion = 0\n";
        config.close();

        std::ofstream index(".aigit/index");
        index.close();

        std::cout << "Initialized empty AI-Git repository in "<< fs::absolute(".aigit") << std::endl;
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Initialization failed: "<< e.what() << std::endl;
    }
}