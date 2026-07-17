#include "commands.hpp"
#include "../core/hashing.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;
using namespace std;

// index is not history it is basically the staging area and only has to store the file you are going to commit the very next
// so if we modify a file, we modify its hash and remove the old hash completely.

namespace Commands
{
    int runAdd(const std::string& filePath)
    {
        if (!fs::exists(".aigit"))
        {
            std::cerr << "Error: Not an AI-Git repository." << std::endl;
            return 1;
        }

        if (!fs::exists(filePath))
        {
            std::cerr << "Error: File does not exist: " << filePath << std::endl;
            return 1;
        }

        if (!fs::is_regular_file(filePath))
        {
            std::cerr << "Error: Path specified is not a regular file: " << filePath << std::endl;
            return 1;
        }

        std::ifstream inFile(filePath, std::ios::binary);

        if (!inFile.is_open())
        {
            std::cerr << "Error: Could not open file." << std::endl;
            return 1;
        }

        std::stringstream buffer;
        buffer << inFile.rdbuf();

        std::string fileContent = buffer.str();
        inFile.close();

        std::string gitHeader = "blob " + std::to_string(fileContent.size()) + '\0';
        std::string storePayload = gitHeader + fileContent;

        std::string sha256Hash = Core::calcSHA256(storePayload);

        if (sha256Hash.empty())
        {
            std::cerr << "Error: Cryptographic hashing mechanism failed." << std::endl;
            return 1;
        }

        std::string dirPrefix = sha256Hash.substr(0, 2);
        std::string fileSuffix = sha256Hash.substr(2);

        fs::path objectsRoot = ".aigit/objects";
        fs::path objectFolder = objectsRoot / dirPrefix;
        fs::path objectFile = objectFolder / fileSuffix;

        try
        {
            fs::create_directories(objectFolder);

            if (!fs::exists(objectFile))
            {
                std::ofstream outFile(objectFile, std::ios::binary);

                if (!outFile.is_open())
                {
                    std::cerr << "Error: Failed to create object." << std::endl;
                    return 1;
                }

                outFile << storePayload;
                outFile.close();
            }

          
            //update staging index with the new/modified file and its hash

            std::unordered_map<std::string, std::string> indexEntries;

            std::ifstream indexIn(".aigit/index");

            if (indexIn.is_open())
            {
                std::string hash, path;

                while (indexIn >> hash >> path)
                {
                    indexEntries[path] = hash;
                }

                indexIn.close();
            }

            indexEntries[filePath] = sha256Hash;

            std::ofstream indexOut(".aigit/index", std::ios::trunc);

            if (!indexOut.is_open())
            {
                std::cerr << "Error: Could not open index." << std::endl;
                return 1;
            }

            for (const auto& entry : indexEntries)
            {
                indexOut << entry.second << " " << entry.first << '\n';
            }

            indexOut.close();

            std::cout << "Added " << filePath << " to staging area." << std::endl;

            return 0;
        }
        catch (const fs::filesystem_error& e)
        {
            std::cerr << "Filesystem Error: " << e.what() << std::endl;
            return 1;
        }
    }
}