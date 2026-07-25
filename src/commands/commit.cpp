#include "commands.hpp"
#include "../core/hashing.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>

//map to store filename and hash

namespace fs = std::filesystem;
using namespace std;

// index is not history it is basically the staging area and only has to store the file you are going to commit the very next
// so if we modify a file, we modify its hash and remove the old hash completely.

namespace Commands
{
    int runCommit()
    {
        struct TreeNode
{
    std::string name;
    bool isDirectory;

    std::string hash;

    std::map<std::string, std::unique_ptr<TreeNode>> children;

    TreeNode(const std::string& nodeName, bool directory)
        : name(nodeName), isDirectory(directory) {}
};

std::ifstream indexFile(".aigit/index");

if (!indexFile.is_open())
{
    std::cerr << "Could not open index." << std::endl;
    return nullptr;
}

std::string hash;
std::string path;

while(indexFile >> hash >> path)
{
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;

    while (getline(ss, part, '/'))
{
    parts.push_back(part);
}
}
    }


}