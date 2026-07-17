#include "core/storage.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include<

namespace fs=std::filesystem;

namespace Core{
        //object path for the other two funcs
        fs::path getObjectPath(const std::string& sha256Hash){
            fs::path objectsRoot=".git/objects";
            
            //if invalid or short hash
            if(sha256Hash.size()<2) {
                return fs::path();
            }

            std::string dirPrefix=sha256Hash.substr(0, 2);
            std::string fileSuffix=sha256Hash.substr(2);

            //overloading of slash for building paths
            return objectsRoot / dirPrefix / fileSuffix;
        }

        bool writeObject(const std::string& sha256Hash, const std::string& compressedData){
            fs::path objectFile=getObjectPath(sha256Hash);
            //empty object file
            if(objectFile.empty()){
                std::cerr<< "Storage Error: Invalid hash provided for storage routing."<< std::endl;
                return false;
            }

            //store the target file path's parent dir
            fs::path objectFolder=objectFile.parent_path();

            try{
                //ensure dir exists inside the database
                fs::create_directories(objectFolder);

                //open file in binary mode to write compressed structures
                std::ofstream outFile(objectFile, std::ios::binary);
                if(!outFile.is_open()){
                    std::cerr<< "Storage Error: Failed to open target path for writing: "<< objectFile<< std::endl;
                    return false;
                }

                //close the outFile stream
                outFile<< compressedData;
                outFile.close();
                return true;

            } //error handling 
            catch(const fs::filesystem_error& e){
                std::cerr<< "Storage Write Exception: "<< e.what()<< std::endl;
                return false;
            }
        }

        std::string readObject(const std::string& sha256Hash){
            fs::path objectFile=getObjectPath(sha256Hash);

            //not regular file means system file
            if(!fs::exists(objectFile) || !fs::is_regular_file(objectFile)){
                std::cerr<< "Storage Error: Object database node not found for hash identity: "<< sha256Hash<< std::endl;
                return "";
            }

            //open a stream for data exchange named inFile in binary mode
            std::ifstream inFile(objectFile, std::ios::binary);
            if (!inFile.is_open()) {
                std::cerr<< "Storage Error: Failed to acquire stream handle for object: "<< objectFile<< std::endl;
                return "";
            }

            //put data in RAM buffer using stringstream and in the read mode of buffer
            std::stringstream buffer;
            buffer<< inFile.rdbuf();
            inFile.close();

            return buffer.str();
        }
}