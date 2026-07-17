#include "commands.hpp"
#include "../core/hashing.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;
using namespace std;

namespace Commands
{
    int runStatus()
    {
        if (!fs::exists(".aigit"))
        {
            std::cerr << "Error: Not an AI-Git repository.\n";
            return 1;
        }

        

        std::unordered_map<std::string, std::string> indexEntries;

        std::ifstream index(".aigit/index");

        if (index.is_open())
        {
            std::string hash, path;

            while (index >> hash >> path)
            {
                indexEntries[path] = hash;
            }

            index.close();
        }

        std::vector<std::string> modifiedFiles;
        std::vector<std::string> deletedFiles;
        std::vector<std::string> untrackedFiles;

       

        for (const auto& entry : fs::recursive_directory_iterator("."))
        {
            if (!entry.is_regular_file())
                continue;

            // Skip repository metadata
            if (entry.path().string().find(".aigit") != std::string::npos)
                continue;

            std::string filePath = entry.path().string();

            // Remove leading "./"
            if (filePath.substr(0,2) == "./")
                filePath = filePath.substr(2);

            

            if (indexEntries.find(filePath) == indexEntries.end())
            {
                untrackedFiles.push_back(filePath);
                continue;
            }

           

            std::ifstream inFile(filePath, std::ios::binary);

            std::stringstream buffer;
            buffer << inFile.rdbuf();

            std::string fileContent = buffer.str();

            std::string gitHeader =
                "blob " + std::to_string(fileContent.size()) + '\0';

            std::string payload = gitHeader + fileContent;

            std::string currentHash = Core::calcSHA256(payload);

           

            if (currentHash != indexEntries[filePath])
            {
                modifiedFiles.push_back(filePath);
            }
        }


        for (const auto& file : indexEntries)
        {
            if (!fs::exists(file.first))
            {
                deletedFiles.push_back(file.first);
            }
        }


        if (modifiedFiles.empty() &&
            deletedFiles.empty() &&
            untrackedFiles.empty())
        {
            std::cout << "Working tree clean.\n";
            return 0;
        }

        if (!modifiedFiles.empty())
        {
            std::cout << "Modified files:\n";

            for (const auto& file : modifiedFiles)
            {
                std::cout << "    " << file << '\n';
            }

            std::cout << '\n';
        }

        if (!deletedFiles.empty())
        {
            std::cout << "Deleted files:\n";

            for (const auto& file : deletedFiles)
            {
                std::cout << "    " << file << '\n';
            }

            std::cout << '\n';
        }

        if (!untrackedFiles.empty())
        {
            std::cout << "Untracked files:\n";

            for (const auto& file : untrackedFiles)
            {
                std::cout << "    " << file << '\n';
            }

            std::cout << '\n';
        }

        return 0;
    }
}