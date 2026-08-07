#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <string>
#include "filesystem.hpp"

namespace Core{
    namespace Storage{
        bool writeObject(const std::string& sha256hash, const std::string& compressedData);
        std::string readObject(const std::string& sha256hash);
        std::filesystem::path getObjectPath(const std::string& sha256hash);
    }
}

#endif