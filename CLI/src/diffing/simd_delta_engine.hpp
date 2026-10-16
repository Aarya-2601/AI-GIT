#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>

namespace Core::Diffing {

enum DeltaOpCode : uint8_t {
    OP_COPY = 0x01, // Copy slice from base chunk: uint32 offset, uint32 length
    OP_ADD  = 0x02  // Insert literal payload: uint32 length, raw bytes
};

struct DeltaPatch {
    std::string baseChunkId;
    size_t targetRawSize{0};
    size_t baseRawSize{0};
    std::vector<uint8_t> patchBytes;
    double compressionRatio{1.0};
};

class SIMDDeltaEngine {
public:
    // Vectorized Gdelta binary delta compressor using SIMD block matching
    static DeltaPatch encodeDeltaSIMD(
        const uint8_t* base,
        size_t baseSize,
        const uint8_t* target,
        size_t targetSize,
        const std::string& baseId = ""
    );

    // Blazing-fast delta reconstruction via RAM memory copies (30-40 GB/s)
    static std::vector<uint8_t> decodeDeltaSIMD(
        const uint8_t* base,
        size_t baseSize,
        const DeltaPatch& patch
    );
};

} // namespace Core::Diffing
