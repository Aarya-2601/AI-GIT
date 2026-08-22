#include "add.hpp"
#include "../core/filesystem.hpp"
#include "../core/hashing.hpp"
#include "../storage/storage_manager.hpp"

//map to store filename and hash

namespace fs=std::filesystem;
using namespace std;

// index is not history it is basically the staging area and only has to store the file you are going to commit the very next
// so if we modify a file, we modify its hash and remove the old hash completely.

namespace Commands
{   
    static std::string normalizePath(const fs::path& p){
        std::string pathStr=p.generic_string();  //converts backslashes like in Windows to forward slashes for uniformity, between paths
        if(pathStr.rfind("./", 0) ==0){
            pathStr=pathStr.substr(2);  //rfind=reverse find, for realtive paths, it starts at index zero and searches backwards, if it finds 
            //./then it changes the relative paths to just the path without ./
        }
        return pathStr;
    }

    static bool processfile(const fs::path& filePath, Core::Index& indexEntries)
    {
        std::string normPath=normalizePath(filePath);

        if(normPath.find(".aigit") != std::string::npos || 
           normPath.find("build/") != std::string::npos || 
           normPath.find(".git") != std::string::npos || 
           normPath.find(".vscode/") != std::string::npos || 
           normPath.find("vcpkg/") != std::string::npos)
        {
            return true;
        }  //ignores if belongs to this category

        try {
            // StorageManager automatically handles FastCDC chunking for files >256KB
            // and monolithic blob storage for files <=256KB, returning the manifest/blob object ID
            Storage::StorageManager storageManager(".aigit");
            std::string objectId = storageManager.storeFile(filePath);

            if (objectId.empty()) {
                std::cerr << "Error: Storage manager failed to store object for: " << normPath << std::endl;
                return false;
            }

            const auto& existingEntries = indexEntries.getEntries();  //looks up the hash in the entries
            auto it = existingEntries.find(normPath);
            if(it != existingEntries.end() && it->second.hash == objectId){
                return true; // File content hasn't changed; skip quietly
            }

            indexEntries.addEntry(Core::IndexEntry(normPath, objectId, "100644"));  //posix mode: for file permissions, standard non executable file
            std::cout<<"Added "<<normPath<<" to staging area."<<std::endl;
            return true;
        }
        catch(const std::exception& e) {
            std::cerr << "Storage Error on " << normPath << ": " << e.what() << std::endl;
            return false;
        }
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
        
        Core::Index indexEntries;  //loads index
        indexEntries.load(".aigit/index");
        
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
                    if(pStr.find(".aigit") != std::string::npos || pStr.find("build/") != std::string::npos || pStr.find(".git") != std::string::npos ||pStr.find(".vscode/") != std::string::npos || pStr.find("vcpkg/") != std::string::npos ){
                        continue;
                    }
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
        //save the updated index entries back to the index
        indexEntries.save(".aigit/index");
        return 0;
    }

    int runAdd(const std::string& filePath){
        return runAdd(std::vector<std::string>{filePath});
    }
}