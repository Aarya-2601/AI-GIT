#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct Chunk
{
    size_t offset;
    size_t length;
    std::string sha256;
};

class Chunking
{
public:
    static constexpr size_t MIN_SIZE = 256 * 1024;
    static constexpr size_t AVG_SIZE = 1024 * 1024;
    static constexpr size_t MAX_SIZE = 4 * 1024 * 1024;

    static constexpr uint64_t MASK_S = 0x001FFFFFULL;
    static constexpr uint64_t MASK_L = 0x0007FFFFULL;

    static std::vector<Chunk> chunkBuffer(
        const uint8_t* data,
        size_t size
    );

    static std::vector<Chunk> chunkFile(
        const std::string& filePath
    );
};