#pragma once

#include "../hashing/xxhash_engine.hpp"
#include "../inspection/file_inspector.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <filesystem>

namespace Core::Chunking {

struct TensorChunk {
    size_t chunkIndex{0};
    size_t offset{0};
    size_t length{0};
    std::string tensorName;     // e.g. "model.layers.0.self_attn.q_proj.weight" or "__header__"
    std::string dtype;          // "F32", "F16", "BF16", "I8", or "RAW"
    std::vector<int64_t> shape;
    Hashing::Hash128 xxhash128; // Blazing 30 GB/s leaf hash
    std::string sha256;         // Content-addressed key
    bool isMetadata{false};
};

struct FastCDCProfile {
    size_t minSize{256 * 1024};      // 256 KB
    size_t avgSize{1024 * 1024};     // 1 MB
    size_t maxSize{4 * 1024 * 1024};  // 4 MB
    uint32_t maskS{0x0003FFFF};
    uint32_t maskL{0x00007FFF};
};

class TensorAwareFastCDC {
public:
    // Chunks any memory-mapped file (20GB+) aligning boundaries to tensor matrices
    static std::vector<TensorChunk> chunkMappedFile(
        const uint8_t* data,
        size_t size,
        const std::filesystem::path& filePath,
        const FastCDCProfile& profile = { 256*1024, 1024*1024, 4*1024*1024 }
    );

    // FastCDC window scanner within bounded memory range [start, end)
    static std::vector<size_t> findFastCDCCutPoints(
        const uint8_t* data,
        size_t start,
        size_t length,
        const FastCDCProfile& profile
    );
};

} // namespace Core::Chunking
