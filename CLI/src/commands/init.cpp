#include "init.hpp"
#include "../core/filesystem.hpp"
#include <iostream>
#include <fstream>
#include "../storage/storage_manager.hpp"

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

Storage::StorageManager storage(".aigit");
storage.initialize();

        std::ofstream config(".aigit/config");
        //section header that groups all low level Git functions interacting with the filesystem header
        config << "[core]\n";
        //version decided for repo, so higher versions will be cautious
        config << "\trepositoryformatversion = 0\n";
        config.close();


std::ofstream index(".aigit/index"); // staging area, stores filees to be committed, so basically files on which git add is run
index.close();


std::cout << "Initialized empty AI-Git repository in "<< fs::absolute(".aigit") << std::endl;
}

catch (const fs::filesystem_error& e) //Give me a readonly reference to the exception object
{
std::cerr << "Initialization failed: "<< e.what() << std::endl;
}


return 0;
}
}