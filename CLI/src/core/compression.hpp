#ifndef COMPRESSION_HPP
#define COMPRESSION_HPP

#include <string>

namespace Core
{
    std::string compressString(const std::string& data);

    // Same as compressString, but with an explicit zlib level (1-9).
    std::string compressString(const std::string& data, int level);

    std::string decompressData(const std::string& compressedData);
}

#endif