#include "commands.hpp"
#include "../core/hashing.hpp"
#include "../core/storage.hpp"
#include "../core/compression.hpp"
#include "../models/blob.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
<<<<<<< HEAD
#include <unordered_map>
=======
#include <stdexcept>
#include <exception>
>>>>>>> f81f17cb9740dd59ec61237e5b7ee89be3b56cc4

namespace fs = std::filesystem;
using namespace std;

<<<<<<< HEAD
// index is not history it is basically the staging area and only has to store the file you are going to commit the very next
// so if we modify a file, we modify its hash and remove the old hash completely.

namespace Commands
{
    int runAdd(const std::string& filePath)
    {
        if (!fs::exists(".aigit"))
        {
=======
namespace Commands{
    int runAdd(const std::string& filePath){
        
        //verify whether repository exists
        if (!fs::exists(".aigit")){
>>>>>>> f81f17cb9740dd59ec61237e5b7ee89be3b56cc4
            std::cerr << "Error: Not an AI-Git repository." << std::endl;
            return 1;
        }

<<<<<<< HEAD
        if (!fs::exists(filePath))
        {
            std::cerr << "Error: File does not exist: " << filePath << std::endl;
            return 1;
        }

        if (!fs::is_regular_file(filePath))
        {
=======
        //verify whether file exists on disk
        if (!fs::exists(filePath)){
            std::cerr << "Error: File does not exist: " << filePath << std::endl;  //cerr=cout with error giving capabilities
            return 1;
        }

        //func returns boolean value false if it is a folder or a system file and not a regular file
        if (!fs::is_regular_file(filePath)){
>>>>>>> f81f17cb9740dd59ec61237e5b7ee89be3b56cc4
            std::cerr << "Error: Path specified is not a regular file: " << filePath << std::endl;
            return 1;
        }

        //open the raw file in binary mode, take the data and put it as a chunk into fileContent
        std::ifstream inFile(filePath, std::ios::binary);

        if (!inFile.is_open()){
            std::cerr << "Error: Could not open file." << std::endl;
            return 1;
        }
        std::stringstream buffer;
        buffer << inFile.rdbuf();
<<<<<<< HEAD

        std::string fileContent = buffer.str();
=======
        std::string fileContent= buffer.str();
>>>>>>> f81f17cb9740dd59ec61237e5b7ee89be3b56cc4
        inFile.close();

        try{
            Models::Blob modelBlob(fileContent);
            std::string rawData= modelBlob.serialize();
            std::string sha256Hash=Core::calcSHA256(rawData);
            std::string compressed=Core::compressString(rawData);
            Core::writeObject(sha256Hash, rawData);

<<<<<<< HEAD
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
=======
            if(sha256Hash.empty()){
                std::cerr<< "Error: Model storage failed."<< std::endl;
                return 1;
            }

            std::filesystem::path indexPath= std::filesystem::path(".aigit") / "index";
            std::ofstream index(indexPath, std::ios::app);
            
            if(!index.is_open()){
                std::cerr << "Error: Could not open index." << std::endl;
                return 1;
            }
            index<< sha256Hash<< " "<< filePath.string()<< std::endl;
            index.close();
>>>>>>> f81f17cb9740dd59ec61237e5b7ee89be3b56cc4

            std::cout<< "Added: "<< filePath<< "to the staging ledger."<< std::endl;
            return 0;
        }
        catch(const std::exception& e){
            std::cerr<< "Filesystem Error: "<< e.what()<< std::endl;
            return 1;
        }
    }
}