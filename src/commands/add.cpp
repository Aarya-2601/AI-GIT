#include "commands.hpp"
#include "../core/hashing.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std;

namespace Commands
{
    int runAdd(const std::string& filePath)
    {
        //verify whether repository exists
        if (!fs::exists(".aigit"))
        {
            std::cerr << "Error: Not an AI-Git repository." << std::endl;
            return 1;
        }

        //verify whether file exists on disk
        if (!fs::exists(filePath))
        {
            std::cerr << "Error: File does not exist: " << filePath << std::endl;  //cerr=cout with error giving capabilities
            return 1;
        }

        //func returns boolean value false if it is a folder or a system file and not a regular file
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

            // Store object only if it doesn't already exist
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

            // Append blob information to the staging index
            std::ofstream index(".aigit/index", std::ios::app);

            if (!index.is_open())
            {
                std::cerr << "Error: Could not open index." << std::endl;
                return 1;
            }

            index << sha256Hash << " " << filePath << '\n';
            index.close();

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