#include "../models/blob.hpp"
#include <utility>  //provides general purpose data templates, move
#include <string>
#include <stdexcept>  //can be used only for runtine error
#include "../core/storage.hpp"
#include "../core/compression.hpp"

namespace Models{

    //parameterized constructor
    Blob::Blob(std::string content){
        mcontent=std::move(content);
    }

    //convert to Git format
    std::string Blob::serialize() const{
        std::string header="blob "+std::to_string(mcontent.size());
        header.push_back('\0');
        return header+mcontent;
    }

    Blob Blob::deserialize(const std::string& data)
{
    size_t headerEnd=data.find('\0');

    if(headerEnd==std::string::npos)
    {
        throw std::runtime_error("Invalid blob object.");  //can throw an error only when running
    }

    std::string content=data.substr(headerEnd+1);

    return Blob(content);
}
} 