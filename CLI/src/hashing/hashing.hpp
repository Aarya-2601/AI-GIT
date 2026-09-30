#ifndef HASHING_HPP
#define HASHING_HPP

#include <string>

namespace Core
{
    // Computes SHA-256 hex digest for byte/string content
    std::string calcSHA256(const std::string &content);
}

#endif // HASHING_HPP
