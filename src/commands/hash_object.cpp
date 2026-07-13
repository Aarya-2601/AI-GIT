#include "commands.hpp"
#include "core/hashing.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs=std::filesystem;

namespace Commands{
    int runHashObject(const std::string& filePath){
        
        //verify whether file exists on disk
        if (!fs::exists(filePath)){
            std::cerr<< "Error: File does not exist: "<< filePath<< endl;  //cerr=cout with error giving capabilities
            return 1;
        }
        //func returns boolean value false if it is a folder or a system file and not a regular file
        if (!fs::is_regular_file(filePath)) {
            std::cerr<< "Error: Path specified is not a regular file: "<< filePath<< endl;
            return 1;
        }

        //object inFile created by ifstream
        //OPENS THE FILE IN BINARY MODEEE
        std::ifstream inFile(filePath, std::ios::binary);
        if (!inFile.is_open()) {
            std::cerr<< "Error: Could not open file for reading: "<< filePath<< endl;
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

        //blob=large binary object
        //separates metadata header and file payload so basically like separating 
        //IMPLEMENTATION OF MODEL SUBPARSER INTO HEADING AND CONTENT
        std::string gitHeader="blob "+std::to_string(fileContent.size())+'\0';  //blob=object type identifier, tree=folders, commit=history snapshots
        std::string storePayload= gitHeader+fileContent;  //puts header in front of the text file

        //compute hash using the core module
        std::string sha256Hash=Core::calcSHA256(storePayload);
        if(sha256Hash.empty()){
            std::cerr<< "Error: Cryptographic hashing mechanism failed."<<endl;
            return 1;
        }

        //text slicing for storage: first two characters become the subfolder, remaining 62 become the file name
        fs::path objectsRoot=".git/objects";
        std::string dirPrefix=sha256Hash.substr(0, 2);
        std::string fileSuffix sha256Hash.substr(2);

        fs::path objectFolder= objectsRoot/dirPrefix;
        fs::path objectFile= objectFolder/fileSuffix;

        try {//create 2 char folder inside the database
            fs::create_directories(objectFolder);

            //write the header+file as an object
            std::ofstream outFile(objectFile, std::ios::binary);
            if (!outFile.is_open()) {
                std::cerr<< "Error: Failed to write object path to disk."<< endl;
                return 1;
            }

            outFile<< storePayload;
            outFile.close();

            //print hash string
            std::cout<< sha256Hash<< endl;
            return 0;

        } catch(const fs::filesystem_error& e){
            std::cerr<< "Database Write Exception: "<< e.what()<< endl;
            return 1;
        }
    }
}