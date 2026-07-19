#include "commands.hpp"
#include "../core/hashing.hpp"
#include "../core/storage.hpp"
#include "../core/compression.hpp"
#include "../models/blob.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdexcept>
#include <exception>

namespace fs = std::filesystem;
using namespace std;

namespace Commands{
    int runAdd(const std::string& filePath){
        
        //verify whether repository exists
        if (!fs::exists(".aigit")){
            std::cerr << "Error: Not an AI-Git repository." << std::endl;
            return 1;
        }

        //verify whether file exists on disk
        if (!fs::exists(filePath)){
            std::cerr << "Error: File does not exist: " << filePath << std::endl;  //cerr=cout with error giving capabilities
            return 1;
        }

        //func returns boolean value false if it is a folder or a system file and not a regular file
        if (!fs::is_regular_file(filePath)){
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
        std::string fileContent= buffer.str();
        inFile.close();

        try{
            Models::Blob modelBlob(fileContent);
            std::string rawData= modelBlob.serialize();
            std::string sha256Hash=Core::calcSHA256(rawData);
            std::string compressed=Core::compressString(rawData);
            Core::writeObject(sha256Hash, rawData);

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
            index<< sha256Hash<< " "<< filePath<< std::endl;
            index.close();

            std::cout<< "Added: "<< filePath<< "to the staging ledger."<< std::endl;
            return 0;
        }
        catch(const std::exception& e){
            std::cerr<< "Filesystem Error: "<< e.what()<< std::endl;
            return 1;
        }
    }
}