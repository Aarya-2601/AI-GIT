#include "bitshuffle_zstd.hpp"
#include "compression.hpp"
#include <cstring>
#include <algorithm>

namespace Core::Compression {

void BitshuffleCodec::bitshuffle(const uint8_t* in, uint8_t* out, size_t size, size_t elemSize) {
    if (size == 0 || elemSize == 0) return;
    size_t numElems = size / elemSize;
    if (numElems == 0) {
        std::memcpy(out, in, size);
        return;
    }

    // Transpose element byte-planes first (planar shuffling)
    for (size_t b = 0; b < elemSize; ++b) {
        for (size_t i = 0; i < numElems; ++i) {
            out[b * numElems + i] = in[i * elemSize + b];
        }
    }

    // Copy trailing remainder bytes if size is not an exact multiple of elemSize
    size_t remainder = size % elemSize;
    if (remainder > 0) {
        std::memcpy(out + numElems * elemSize, in + numElems * elemSize, remainder);
    }
}

void BitshuffleCodec::bitunshuffle(const uint8_t* in, uint8_t* out, size_t size, size_t elemSize) {
    if (size == 0 || elemSize == 0) return;
    size_t numElems = size / elemSize;
    if (numElems == 0) {
        std::memcpy(out, in, size);
        return;
    }

    for (size_t b = 0; b < elemSize; ++b) {
        for (size_t i = 0; i < numElems; ++i) {
            out[i * elemSize + b] = in[b * numElems + i];
        }
    }

    size_t remainder = size % elemSize;
    if (remainder > 0) {
        std::memcpy(out + numElems * elemSize, in + numElems * elemSize, remainder);
    }
}

std::vector<uint8_t> BitshuffleCodec::compressFloatWeights(
    const uint8_t* data,
    size_t size,
    size_t elemSize
) {
    if (!data || size == 0) return {};

    std::vector<uint8_t> shuffled(size);
    bitshuffle(data, shuffled.data(), size, elemSize);

    std::string inStr(reinterpret_cast<const char*>(shuffled.data()), size);
    std::string compressed = Core::compressString(inStr);

    return std::vector<uint8_t>(compressed.begin(), compressed.end());
}

std::vector<uint8_t> BitshuffleCodec::decompressFloatWeights(
    const uint8_t* compressedData,
    size_t compressedSize,
    size_t rawSize,
    size_t elemSize
) {
    if (!compressedData || compressedSize == 0 || rawSize == 0) return {};

    std::string compStr(reinterpret_cast<const char*>(compressedData), compressedSize);
    std::string decompressed = Core::decompressData(compStr);

    if (decompressed.size() < rawSize) return {};

    std::vector<uint8_t> restored(rawSize);
    bitunshuffle(reinterpret_cast<const uint8_t*>(decompressed.data()), restored.data(), rawSize, elemSize);

    return restored;
}

} // namespace Core::Compression
