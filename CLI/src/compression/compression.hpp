#ifndef COMPRESSION_HPP
#define COMPRESSION_HPP

#include <string>

namespace Core
{
    // Standard zlib deflate compression
    std::string compressString(const std::string& data);

    // Standard zlib inflate decompression
    std::string decompressData(const std::string& compressedData);
}

#endif // COMPRESSION_HPP
