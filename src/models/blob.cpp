#include <models/blob.hpp>
#include <utility>

namespace Models{

    //parameterized constructor
    Blob::Blob(std::string content){
        mcontent =std::move(content);
    }

    //convert to Git format
    std::string Blob::serialize() const{
        std::string header= "blob "+std::to_string(mcontent.size());
        header.push_back('\0');
        return header+mcontent;
    }
} 