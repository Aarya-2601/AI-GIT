#ifndef CORE_FILESYSTEM_HPP
#define CORE_FILESYSTEM_HPP

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace Core
{
    // Reads an entire file into memory as a binary-safe string. Throws
    // std::runtime_error if the file cannot be opened. Shared by the
    // handful of call sites (object storage read/write, hash-object,
    // status's per-file hash check) that previously each declared their
    // own ifstream+stringstream buffering copy of this.
    std::string readFileToString(const std::filesystem::path& path);
}

#endif