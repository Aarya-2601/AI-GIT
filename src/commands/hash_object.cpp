#include "commands.hpp"
#include "core/hashing.hpp"
#include "core/compression.hpp"
#include "core/storage.hpp"
#include "models/blob.hpp"
#include "core/filesystem.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace Commands{

    int runHashObject(const std::string& filePath){
        //verify whether file exists on disk
        if (!fs::exists(filePath)){
            std::cerr<< "Error: File does not exist: "<< filePath<< std::endl;  //cerr=cout with error giving capabilities
            return 1;
        }
        //func returns boolean value false if it is a folder or a system file and not a regular file
        if (!fs::is_regular_file(filePath)){
            std::cerr<< "Error: Path specified is not a regular file: "<< filePath<< std::endl;
            return 1;
        }

        //object inFile created by ifstream
        //OPENS THE FILE IN BINARY MODEEE
        std::ifstream inFile(filePath, std::ios::binary);
        if (!inFile.is_open()){
            std::cerr<< "Error: Could not open file for reading: "<< filePath<< std::endl;
            return 1;
        }

        //buffer named empty string stream created
        std::stringstream buffer;
        //reads buffer and returns pointer to it
        buffer<< inFile.rdbuf();
        //packages buffer content to string
        std::string fileContent=buffer.str();
        //breaks connection between hard drive and program
        inFile.close();

        Models::Blob blobObject(fileContent);
        std::string storePayload= blobObject.serialize(); 

        //compute hash using the core module
        std::string sha256Hash=Core::calcSHA256(storePayload);
        if(sha256Hash.empty()){
            std::cerr<< "Error: Cryptographic hashing mechanism failed."<< std::endl;
            return 1;
        }

        try{
            //deflate via zli
            std::string compressedPayload=Core::compressString(storePayload);

            if(!Core::writeObject(sha256hash, compressedPayload)){
                return 1;
            }
            //print hash string
            std::cout<< sha256Hash<< std::endl;
            return 0;

        } 
        catch(const std::exception& e){
            std::cerr<< "Execution Exception: "<< e.what()<< std::endl;
            return 1;
        }
    }
}