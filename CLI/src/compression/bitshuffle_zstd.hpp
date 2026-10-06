#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>

namespace Core::Compression {

class BitshuffleCodec {
public:
    // Transposes bit-planes across floating-point weights and compresses
    static std::vector<uint8_t> compressFloatWeights(
        const uint8_t* data,
        size_t size,
        size_t elemSize = 4 // 4 for FP32, 2 for FP16/BF16
    );

    // Decompresses and restores bit-exact float weights
    static std::vector<uint8_t> decompressFloatWeights(
        const uint8_t* compressedData,
        size_t compressedSize,
        size_t rawSize,
        size_t elemSize = 4
    );

    // Low-level bit-plane transposition
    static void bitshuffle(const uint8_t* in, uint8_t* out, size_t size, size_t elemSize);
    static void bitunshuffle(const uint8_t* in, uint8_t* out, size_t size, size_t elemSize);
};

} // namespace Core::Compression
