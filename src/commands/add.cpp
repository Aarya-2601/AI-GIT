#include "commands.hpp"
#include "core/hashing.hpp"
#include "core/filesystem.hpp"
#include "commands/add.hpp"
#include "models/blob.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>

//map to store filename and hash

namespace fs=std::filesystem;
using namespace std;

// index is not history it is basically the staging area and only has to store the file you are going to commit the very next
// so if we modify a file, we modify its hash and remove the old hash completely.

namespace Commands
{   
    static std::string normalizePath(const fs::path& p){
        std::string pathStr=p.generic_string();
        if(pathStr.rfind("./", 0) ==0){
            pathStr=pathStr.substr(2);
        }
        return pathStr;
    }

    static bool processfile(const fs::path& filePath, std::unordered_map<std::string, std::string>& indexEntries){
        std::ifstream inFile(filePath, std::ios::binary);
        if(!inFile.is_open()){
            std::cerr<<"Error: Could not open file: "<<std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer<<inFile.rdbuf();
        std::string fileContent=buffer.str();
        inFile.close();

        Models::Blob blobObject(fileContent);
        std::string storePayload= blobObject.serialize();
        std::string sha256Hash= Core::calcSHA256(storePayload);
        if(sha256Hash.empty()){
            std::cerr<< "Error: Cryptographic hashing mechanism failed."<< std::endl;
            return false;
        }

        std::string dirPrefix=sha256Hash.substr(0, 2);
        std::string fileSuffix=sha256Hash.substr(2);

        fs::path objectFolder=fs::path(".aigit/objects")/dirPrefix;
        fs::path objectFile=objectFolder/fileSuffix;

        try
        {
            fs::create_directories(objectFolder);

            if (!fs::exists(objectFile))
            {
                std::ofstream outFile(objectFile, std::ios::binary);

                if (!outFile.is_open())
                {
                    std::cerr << "Error: Failed to create object." << std::endl;
                    return false;
                }

                outFile << storePayload;
                outFile.close();
            }
        }
        catch(const fs::filesystem_error& e){
            std::cerr<<"Filesystem Error: "<<e.what()<<std::endl;
            return false;
        }
            
        std::string normPath=normalizePath(filePath);
        indexEntries[normPath]=sha256Hash;
        std::cout<<"Added "<<normPath<<" to staging area."<<std::endl;
        return true;
    }

    int runAdd(const std::vector<std::string>& targets)
    {
        if (!fs::exists(".aigit")) //does .aigit exist?
        {
            std::cerr << "Error: Not an AI-Git repository." << std::endl;
            return 1;
        }

        if(targets.empty()){
            std::cerr << "Nothing specified, nothing added." << std::endl;
            return 0;
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

        for(const auto& target : targets){
            fs::path targetPath(target);
            if(!fs::exists(targetPath)){
                std::cerr<<"Error: Path does not exist: "<<target<<std::endl;
                continue;
            }
            //if target is a directory
            if(fs::is_directory(targetPath)){
                for(const auto& entry : fs::recursive_directory_iterator(targetPath)){
                    std::string pStr=entry.path().generic_string();

                    //skip aigit folder
                    if(pStr.find(".aigit") != std::string::npos)
                        continue;

                    //process regular files inside sub-directories
                    if(fs::is_regular_file(entry.status())){
                        processfile(entry.path(), indexEntries);
                    }
                }
            }
            //if target is a single file
            else if (fs::is_regular_file(targetPath)){
                processfile(targetPath, indexEntries);
            }
            else{
                std::cerr<<"Warning: Skipping unsupported path: "<<target<<std::endl;
            }
        }

        std::ofstream indexOut(".aigit/index", std::ios::trunc);
        if(!indexOut.is_open())
        {
            std::cerr << "Error: Could not open index." << std::endl;
            return 1;
        }

        for (const auto& entry : indexEntries)
        {
            indexOut << entry.second << " " << entry.first << '\n';
        }

        indexOut.close();
        return 0;
    }

    int runAdd(const std::string& filePath){
        return runAdd(std::vector<std::string>{filePath});
    }
}