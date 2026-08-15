#include "../models/blob.hpp"
#include <utility>
#include <string>
#include "../models/blob.hpp"
#include <utility>
#include <string>
#include <stdexcept>
#include "../core/storage.hpp"
#include "../core/compression.hpp"

//the purpose of this is basically making the blob object which is like a box and then serialize it so we can hash it

namespace Models
{

    //parameterized constructor
    Blob::Blob(std::string content)
    {
        mcontent =std::move(content); //moves content, not copy
    }

    //convert to Git format
    std::string Blob::serialize() const
    {
        std::string header= "blob "+std::to_string(mcontent.size());
        header.push_back('\0');
        return header+mcontent;
    }

    Blob Blob::deserialize(const std::string& data)
    {
    size_t headerEnd = data.find('\0');

    if (headerEnd == std::string::npos)
    {
        throw std::runtime_error("Invalid blob object.");
    }

    std::string content =data.substr(headerEnd + 1);

    return Blob(content);
    }


} 