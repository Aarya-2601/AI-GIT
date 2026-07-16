  #ifndef STORAGE_HPP
  #define STORAGE_HPP

  #include <string>
  #include "core/filesystem.hpp"

  namespace Core{
      bool writeObject(const std::string& sha256hash, const std::string& compressedData);
      bool readObject(const std::string& sha256hash);
      std::fs::path getObjectPath(const std::string& sha256hash);
  }

  #endif