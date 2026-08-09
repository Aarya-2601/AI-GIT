#ifndef COMPRESSION_HPP
#define COMPRESSION_HPP

#include <string>

namespace Core{
    std::string compressString(const std::string& data);
    std::string decompressData(const std::string& compressedData);
}

#endif