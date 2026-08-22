#include "../models/blob.hpp"
#include <utility>  //provides general purpose data templates, move
#include <string>
#include <stdexcept>  //can be used only for runtine error
#include "../core/storage.hpp"
#include "../core/compression.hpp"

//the purpose of this is basically making the blob object which is like a box and then serialize it so we can hash it

namespace Models
{

    //parameterized constructor
<<<<<<< HEAD
    Blob::Blob(std::string content)
    {
        mcontent =std::move(content); //moves content, not copy
    }

    //convert to Git format
    std::string Blob::serialize() const
    {
        std::string header= "blob "+std::to_string(mcontent.size());
=======
    Blob::Blob(std::string content){
        mcontent=std::move(content);
    }

    //convert to Git format
    std::string Blob::serialize() const{
        std::string header="blob "+std::to_string(mcontent.size());
>>>>>>> aaryascrazycommits
        header.push_back('\0');
        return header+mcontent;
    }

    Blob Blob::deserialize(const std::string& data)
<<<<<<< HEAD
    {
    size_t headerEnd = data.find('\0');
=======
{
    size_t headerEnd=data.find('\0');
>>>>>>> aaryascrazycommits

    if(headerEnd==std::string::npos)
    {
        throw std::runtime_error("Invalid blob object.");  //can throw an error only when running
    }

<<<<<<< HEAD
    std::string content =data.substr(headerEnd + 1);

    return Blob(content);
    }


=======
    std::string content=data.substr(headerEnd+1);

    return Blob(content);
}
>>>>>>> aaryascrazycommits
} 