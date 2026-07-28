#include "commands/add.hpp"

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

    static bool processfile(const fs::path& filePath, Core::Index& indexEntries){
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

        try{
            std::string compressedData=Core::compressString(storePayload);
            if(!Core::Storage::writeObject(sha256Hash, compressedData)){
                std::cerr << "Error: Storage system failed to write object blob." << std::endl;
                return false;
            }
        }
        catch(const std::exception& e){
            std::cerr<<"Compression/Storage Error: "<<e.what()<<std::endl;
            return false;
        }

        std::string normPath=normalizePath(filePath);
        indexEntries.addEntry(Core::IndexEntry(normPath, sha256Hash, "100644"));
        std::cout<<"\nAdded "<<normPath<<" to staging area."<<std::endl<<std::endl;
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
        
        Core::Index indexEntries;
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

        indexEntries.save(".aigit/index");
        return 0;
    }

    int runAdd(const std::string& filePath){
        return runAdd(std::vector<std::string>{filePath});
    }
}