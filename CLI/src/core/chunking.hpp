#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

struct Chunk{
    size_t offset;
    size_t length;
    std::string sha256;
};

class Chunking{
public:
    // Boundary configurations for max 4MB chunks
    static constexpr size_t MIN_SIZE = 256 * 1024;        // 256 KB floor
    static constexpr size_t AVG_SIZE = 1024 * 1024;       // 1 MB target
    static constexpr size_t MAX_SIZE = 4 * 1024 * 1024;   // 4 MB ceiling

    // Normalized masks (Target: 1MB = 2^20 bits)
    // MASK_S checks 21 bits (0x001FFFFF) in [MIN, AVG]
    // MASK_L checks 19 bits (0x0007FFFF) in [AVG, MAX]
    static constexpr uint64_t MASK_S = 0x001FFFFFULL;
    static constexpr uint64_t MASK_L = 0x0007FFFFULL;

    // Splits an in-memory buffer into variable-sized deduplicated chunks
    static std::vector<Chunk> chunkBuffer(const uint8_t* data, size_t size);
    
    // Memory-mapped file chunker for 5GB+ binary files
    static std::vector<Chunk> chunkFile(const std::string& filePath);
};