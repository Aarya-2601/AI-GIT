#include "mmap_file.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <iostream>
#include <algorithm>

namespace Core {

MMapFile::MMapFile() = default;

MMapFile::~MMapFile() {
    close();
}

MMapFile::MMapFile(MMapFile&& other) noexcept
    : path_(std::move(other.path_)),
      data_(other.data_),
      size_(other.size_)
#if defined(_WIN32)
    , fileHandle_(other.fileHandle_),
      mappingHandle_(other.mappingHandle_)
#else
    , fd_(other.fd_)
#endif
{
    other.data_ = nullptr;
    other.size_ = 0;
#if defined(_WIN32)
    other.fileHandle_ = nullptr;
    other.mappingHandle_ = nullptr;
#else
    other.fd_ = -1;
#endif
}

MMapFile& MMapFile::operator=(MMapFile&& other) noexcept {
    if (this != &other) {
        close();
        path_ = std::move(other.path_);
        data_ = other.data_;
        size_ = other.size_;
#if defined(_WIN32)
        fileHandle_ = other.fileHandle_;
        mappingHandle_ = other.mappingHandle_;
        other.fileHandle_ = nullptr;
        other.mappingHandle_ = nullptr;
#else
        fd_ = other.fd_;
        other.fd_ = -1;
#endif
        other.data_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

bool MMapFile::open(const std::filesystem::path& filePath, bool sequentialHint) {
    close();
    path_ = filePath;

#if defined(_WIN32)
    DWORD flags = FILE_ATTRIBUTE_READONLY;
    if (sequentialHint) {
        flags |= FILE_FLAG_SEQUENTIAL_SCAN;
    }

    HANDLE hFile = CreateFileW(
        filePath.wstring().c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        flags,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    fileHandle_ = hFile;

    LARGE_INTEGER li;
    if (!GetFileSizeEx(hFile, &li)) {
        close();
        return false;
    }
    size_ = static_cast<size_t>(li.QuadPart);
    if (size_ == 0) {
        return true;
    }

    HANDLE hMap = CreateFileMappingW(
        hFile,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
    );

    if (!hMap) {
        close();
        return false;
    }
    mappingHandle_ = hMap;

    void* ptr = MapViewOfFile(
        hMap,
        FILE_MAP_READ,
        0,
        0,
        0 // Map entire file in 64-bit virtual space
    );

    if (!ptr) {
        close();
        return false;
    }

    data_ = static_cast<const uint8_t*>(ptr);
    return true;

#else
    int flags = O_RDONLY;
    fd_ = ::open(filePath.c_str(), flags);
    if (fd_ < 0) return false;

    struct stat st;
    if (fstat(fd_, &st) != 0) {
        close();
        return false;
    }
    size_ = static_cast<size_t>(st.st_size);
    if (size_ == 0) return true;

    void* ptr = ::mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd_, 0);
    if (ptr == MAP_FAILED) {
        close();
        return false;
    }
    data_ = static_cast<const uint8_t*>(ptr);
    if (sequentialHint) {
        posix_madvise(const_cast<uint8_t*>(data_), size_, POSIX_MADV_SEQUENTIAL);
    }
    return true;
#endif
}

void MMapFile::close() {
#if defined(_WIN32)
    if (data_) {
        UnmapViewOfFile(data_);
        data_ = nullptr;
    }
    if (mappingHandle_) {
        CloseHandle(static_cast<HANDLE>(mappingHandle_));
        mappingHandle_ = nullptr;
    }
    if (fileHandle_ && fileHandle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(static_cast<HANDLE>(fileHandle_));
        fileHandle_ = nullptr;
    }
#else
    if (data_) {
        ::munmap(const_cast<uint8_t*>(data_), size_);
        data_ = nullptr;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
    size_ = 0;
}

void MMapFile::prefetchRange(size_t offset, size_t length) const {
    if (!data_ || offset >= size_) return;
    size_t end = std::min(size_, offset + length);
    volatile uint8_t dummy = 0;
    // Touch 4KB page boundaries to trigger background DMA fault
    for (size_t p = offset; p < end; p += 4096) {
        dummy ^= data_[p];
    }
    (void)dummy;
}

void MMapFile::evictRange(size_t offset, size_t length) const {
    if (!data_ || offset >= size_) return;
    size_t actualLen = std::min(size_ - offset, length);
#if defined(_WIN32)
    VirtualUnlock((LPVOID)(data_ + offset), actualLen);
#else
    posix_madvise(const_cast<uint8_t*>(data_ + offset), actualLen, POSIX_MADV_DONTNEED);
#endif
}

double MMapFile::getCurrentProcessRSS_MB() {
#if defined(_WIN32)
    EmptyWorkingSet(GetCurrentProcess());
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return static_cast<double>(pmc.PrivateUsage) / (1024.0 * 1024.0);
    }
    return 0.0;
#else
    // POSIX fallback
    return 0.0;
#endif
}

} // namespace Core
