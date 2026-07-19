#include "models/tree.hpp"
#include <stdexcept>

namespace Models{

    //why can we not use hashing.cpp in this? It takes raw data, 
    //runs it through the OpenSSL engine, and returns a 64-character readable text string
    //when we add file to the tree, it is already a 64 char hash string which needs to be compressed to 32 bytes
    //so basically, the function will halven the size of the hash string
    static std::string hexToBinary(const std::string& hex){
        if(hex.length()%2 != 0){
            throw std::runtime_error("Malformed hexadecimal string: odd length encountered during tree packaging.");
        }
        
        std::string binaryBytes;
        binaryBytes.reserve(hex.length()/2);
        
        for(size_t i=0; i<hex.length(); i+=2){
            std::string byteString= hex.substr(i, 2);
            //string to integer function using hexadecimal values
            char byte= static_cast<char>(std::stoi(byteString, nullptr, 16));  //converts int of 4 byte to char of 1 byte
            binaryBytes.push_back(byte);
        }
        
        return binaryBytes;
    }

    //adding entry to tree
    void Tree::addEntry(const TreeDef& entry){
        mentries.push_back(entry);
    }

    //converts tree object to flat sequence of binary data
    std::string Tree::serialize() const{
        std::string treeContent;

        //processes every folder or file record entry
        for(const auto& entry:mentries){
            //[mode] [name]\0[hash_bytes]
            treeContent+= entry.mode+" "+entry.name;
            treeContent.push_back('\0'); 
            treeContent+= hexToBinary(entry.hash);
        }

        //tree [body_size]\0
        std::string header= "tree "+std::to_string(treeContent.size());
        header.push_back('\0');

        return header+treeContent;
    }
}

/*POSIX DIRECTORY FILE FORMATS
[type][permissions]
100-normal files
120-symbolic link
040-directory*/