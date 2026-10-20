#pragma once

#include <string>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <filesystem>

namespace Core {

class MMapFile {
public:
    MMapFile();
    ~MMapFile();

    // Disable copy, allow move
    MMapFile(const MMapFile&) = delete;
    MMapFile& operator=(const MMapFile&) = delete;
    MMapFile(MMapFile&& other) noexcept;
    MMapFile& operator=(MMapFile&& other) noexcept;

    // Opens and memory-maps any file (20GB+) with zero heap memory allocation
    bool open(const std::filesystem::path& filePath, bool sequentialHint = true);
    void close();

    bool isOpen() const { return data_ != nullptr; }
    const uint8_t* data() const { return data_; }
    size_t size() const { return size_; }
    const std::filesystem::path& path() const { return path_; }

    // Memory advice to kernel
    void prefetchRange(size_t offset, size_t length) const;
    void evictRange(size_t offset, size_t length) const;

    // Process memory footprint monitor (returns current physical RAM / RSS in MB)
    static double getCurrentProcessRSS_MB();

private:
    std::filesystem::path path_;
    const uint8_t* data_{nullptr};
    size_t size_{0};

#if defined(_WIN32)
    void* fileHandle_{nullptr};    // HANDLE
    void* mappingHandle_{nullptr}; // HANDLE
#else
    int fd_{-1};
#endif
};

} // namespace Core
