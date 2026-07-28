#include "commands.hpp"
#include "commands/status.hpp"
#include "core/hashing.hpp"
#include "core/filesystem.hpp"
#include "models/blob.hpp"

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <set>
#include <algorithm>

namespace fs=std::filesystem;

namespace Commands
{   
    static std::string normalizePath(const fs::path& p){
        std::string pathStr=p.generic_string();
        std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

        if(pathStr.rfind("./", 0) == 0)
        {
            pathStr=pathStr.substr(2);
        }
        //carriage return will not turn into a newline character
        while (!pathStr.empty() && (pathStr.back() == '\r' || pathStr.back() == '\n' || pathStr.back() == ' '))
        {
            pathStr.pop_back();
        }
        return pathStr;
    }

    int runStatus()
    {
        if (!fs::exists(".aigit")) // repository check
        {
            std::cerr << "Error: Not an AI-Git repository.\n";
            return 1;
        }

        std::unordered_map<std::string, std::string> indexEntries;
        std::ifstream index(".aigit/index"); //index file kholo

        if (index.is_open()) 
        {   
            std::string line;
            while (std::getline(index, line)){
                std::string cleanLine = normalizePath(line);
                if (cleanLine.empty()) continue;

                std::stringstream ss(cleanLine);
                std::string hash, path;

                if (ss >> hash >> path) // usme me se sab padho aur map me daal do
                {
                    indexEntries[path] = hash;
                }
            }
            index.close();
        }

        std::vector<std::string> modifiedFiles;
        std::vector<std::string> deletedFiles;
        std::vector<std::string> untrackedFiles;
        std::vector<std::string> stagedFiles;
        std::set<std::string> seenDiskFiles;

        for (const auto& entry : fs::recursive_directory_iterator(".")) // ye aise repository scan karte he
        {      
            if(!entry.is_regular_file()) continue;

            std::string pStr=normalizePath(entry.path().generic_string());
            // Skip repository metadata
            if (pStr.find(".aigit") != std::string::npos || pStr.find("build/") != std::string::npos || pStr.find(".git") != std::string::npos || pStr.find(".vscode/") != std::string::npos || pStr.find("vcpkg/") != std::string::npos)
            {
                continue;
            }
            if (!entry.is_regular_file())
                continue;

            std::string filePath=normalizePath(entry.path());
            seenDiskFiles.insert(filePath);

            // Remove leading "./"
            if (filePath.substr(0,2) == "./")
                filePath = filePath.substr(2);

            // if file is in index, continue, else, put that file into untracked files
            if (indexEntries.find(filePath) == indexEntries.end()) 
            {
                untrackedFiles.push_back(filePath);
                continue;
            }
            else{
                // modified file check agar file is in index, toh blob, hash, compare if hashes are same
                // if not modify hash, put in map
                std::ifstream inFile(entry.path(), std::ios::binary);
                if(inFile.is_open()){
                    std::stringstream buffer;
                    buffer<< inFile.rdbuf();
                    std::string fileContent = buffer.str();
                    inFile.close();

                    Models::Blob blobObject(fileContent);
                    std::string storePayload= blobObject.serialize();
                    std::string sha256Hash= Core::calcSHA256(storePayload);
                    if(sha256Hash.empty()){
                        std::cerr<< "Error: Cryptographic hashing mechanism failed."<< std::endl;
                        return false;
                    }
                    if (sha256Hash != indexEntries[filePath])
                    {
                        modifiedFiles.push_back(filePath);
                    }
                }
                else{
                    std::cerr<<"Error: Failed to open an InFile."<<std::endl;
                }
            }
        }

        for (const auto& file : indexEntries) // now scan index and check if file is still present in working directory, if not, put that file into deleted files
        {
            if (seenDiskFiles.find(file.first) == seenDiskFiles.end())
            {
                deletedFiles.push_back(file.first);
            }
        }

        //formatting
        std::cout<<"On branch main\n\n";  //abhi ke liye main rakha hai, when we get ai-git branch sorted then we will add branch name here
        bool hasChanges=false;

        //staged files
        if (!stagedFiles.empty()){
            hasChanges=true;
            std::cout<<"Changes to be committed:\n";
            for(const auto& file : stagedFiles){
                std::cout<<"\tnew file:   "<<file<<"\n";
            }
            std::cout<<"\n";
        }

        //unstaged modifications and deletions
        if (modifiedFiles.empty() ||
            deletedFiles.empty())
        {
            hasChanges=true;
            std::cout<<"Changes not staged for commit:\n";
            std::cout<<"  (use \"ai-git add <file>...\" to update what will be committed)\n";
            for(const auto& file : modifiedFiles){
                std::cout<<"\tmodified:   "<<file<<"\n";
            }
            for(const auto& file : deletedFiles){
                std::cout<<"\tdeleted:    "<<file<<"\n";
            }
            std::cout<<"\n";
        }

        //untracked files
        if (!untrackedFiles.empty()){
            hasChanges=true;
            std::cout<<"Untracked files:\n";
            std::cout<<"  (use \"ai-git add <file>...\" to include in what will be committed)\n";
            for(const auto& file : untrackedFiles)
            {
                std::cout<<"\t"<<file<<"\n";
            }
            std::cout<<"\n";
        }
        
        if (!hasChanges){
            std::cout<<"Nothing to commit, working tree clean\n";
        }
        else if(stagedFiles.empty() && !untrackedFiles.empty()){
            std::cout<<"Nothing added to commit but untracked files present (use \"ai-git add\" to track)\n";
        }
        else if(stagedFiles.empty() && (!modifiedFiles.empty() || !deletedFiles.empty())){
            std::cout<<"No changes added to commit (use \"ai-git add\" and/or \"ai-git commit\")\n";
        }

        return 0;
    }
}