#include "tree.hpp"
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
        binaryBytes.reserve(hex.length()/2);  //reserve space for 32 char
        
        for(size_t i=0; i<hex.length(); i+=2){
            std::string byteString= hex.substr(i, 2);
            //string to integer function using hexadecimal values
            char byte= static_cast<char>(std::stoi(byteString, nullptr, 16));  //parses 2char into an int btw 0 and 255, passing nullptr=dont have to track the end
            binaryBytes.push_back(byte);  //static_char=4byte int to 1 byte char
        }
        
        return binaryBytes;
    }

    static std::string binaryToHex(const std::string& binary)
{
    static const char hexDigits[] = "0123456789abcdef";

    std::string hex;
    hex.reserve(binary.size() * 2);

    for (unsigned char byte : binary)
    {  //each byte is split into 2 4-bit nibbles
        hex.push_back(hexDigits[(byte >> 4) & 0x0F]);  //isolates the top 4 bits
        hex.push_back(hexDigits[byte & 0x0F]);  //isolates the bottom 4 bits
    }

    return hex;
}

    //adding entry to tree
    void Tree::addEntry(const TreeDef& entry){
        entries.push_back(entry);
    }

    //converts tree object to flat sequence of binary data
    std::string Tree::serialize() const{
        std::string treeContent;

        //processes every folder or file record entry
        for(const auto& entry: entries){
            //[mode] [name]\0[hash_bytes]
            treeContent+= entry.mode+" "+entry.name;  //entry mode info is given below
            treeContent.push_back('\0'); 
            treeContent+= hexToBinary(entry.hash);
        }

        //tree [body_size]\0
        std::string header= "tree "+std::to_string(treeContent.size());
        header.push_back('\0');

        return header+treeContent;
    }

    Tree Tree::deserialize(const std::string& data)
{
    Tree tree;

    size_t headerEnd = data.find('\0');

    if(headerEnd == std::string::npos)
    {
        throw std::runtime_error("Invalid tree object.");
    }

    size_t pos = headerEnd + 1;

    while(pos < data.size())
    {
        size_t spacePos = data.find(' ', pos);

        std::string mode =
            data.substr(pos, spacePos - pos);

        pos = spacePos + 1;

        size_t nullPos = data.find('\0', pos);

        std::string name =
            data.substr(pos, nullPos - pos);

        pos = nullPos + 1;

        std::string hashBytes =
            data.substr(pos, 32);

        pos += 32;

        TreeDef entry;

        entry.mode = mode;
        entry.name = name;
        entry.hash = binaryToHex(hashBytes);
        entry.isSubtree = (mode == "040000");

        tree.addEntry(entry);
    }

    return tree;
}
}

/*POSIX DIRECTORY FILE FORMATS
[type][permissions]
040000-tree, 100644-blob normal file, 100755-blob executable, 120000-symbolic link*/