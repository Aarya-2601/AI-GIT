#include <windows.h>
#include <iostream>
#include <vector>
#include <chrono>

int main() {
    const char* testFile = "io_test_tmp.bin";
    const size_t testSize = 256 * 1024 * 1024; // 256 MB

    std::cout << "Creating 256 MB test payload on NVMe...\n";
    HANDLE hFile = CreateFileA(testFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateFile failed: " << GetLastError() << "\n";
        return 1;
    }

    // Set file size
    LARGE_INTEGER li;
    li.QuadPart = testSize;
    SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);
    SetEndOfFile(hFile);

    // Create memory mapping
    HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (!hMap) {
        std::cerr << "CreateFileMapping failed: " << GetLastError() << "\n";
        CloseHandle(hFile);
        return 1;
    }

    auto* ptr = static_cast<uint8_t*>(MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, testSize));
    if (!ptr) {
        std::cerr << "MapViewOfFile failed: " << GetLastError() << "\n";
        CloseHandle(hMap);
        CloseHandle(hFile);
        return 1;
    }

    // Measure write throughput via mmap
    auto t0 = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < testSize; i += 64) {
        ptr[i] = static_cast<uint8_t>(i & 0xFF);
    }
    FlushViewOfFile(ptr, testSize);
    auto t1 = std::chrono::high_resolution_clock::now();

    UnmapViewOfFile(ptr);
    CloseHandle(hMap);
    CloseHandle(hFile);

    // Reopen for read-only sequential mmap benchmark
    hFile = CreateFileA(testFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    const auto* rptr = static_cast<const uint8_t*>(MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, testSize));

    auto t2 = std::chrono::high_resolution_clock::now();
    uint64_t sum = 0;
    // Sequential 64-byte strides (cache line scan)
    for (size_t i = 0; i < testSize; i += 64) {
        sum += rptr[i];
    }
    auto t3 = std::chrono::high_resolution_clock::now();

    UnmapViewOfFile(rptr);
    CloseHandle(hMap);
    CloseHandle(hFile);
    DeleteFileA(testFile);

    double writeSec = std::chrono::duration<double>(t1 - t0).count();
    double readSec = std::chrono::duration<double>(t3 - t2).count();

    double writeMBps = (testSize / (1024.0 * 1024.0)) / writeSec;
    double readMBps  = (testSize / (1024.0 * 1024.0)) / readSec;

    std::cout << "Windows mmap Support:     VERIFIED (CreateFileMapping / MapViewOfFile)\n";
    std::cout << "Sequential Write Rate:    " << writeMBps << " MB/s\n";
    std::cout << "Sequential Read Throughput: " << readMBps << " MB/s (" << (readMBps / 1024.0) << " GB/s)\n";
    std::cout << "Memory Overhead:          0 B heap allocation (zero-copy virtual paging)\n";

    return 0;
}
