#include "simd_delta_engine.hpp"
#include <unordered_map>
#include <cstring>
#include <algorithm>

namespace Core::Diffing {

static inline uint32_t readU32(const uint8_t* p) {
    uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}

static inline void writeU32(std::vector<uint8_t>& buf, uint32_t v) {
    uint8_t bytes[4];
    std::memcpy(bytes, &v, 4);
    buf.insert(buf.end(), bytes, bytes + 4);
}

DeltaPatch SIMDDeltaEngine::encodeDeltaSIMD(
    const uint8_t* base,
    size_t baseSize,
    const uint8_t* target,
    size_t targetSize,
    const std::string& baseId
) {
    DeltaPatch patch;
    patch.baseChunkId = baseId;
    patch.baseRawSize = baseSize;
    patch.targetRawSize = targetSize;

    // Header: 'G','D','E','L' (4B) + targetSize (4B)
    patch.patchBytes.push_back('G');
    patch.patchBytes.push_back('D');
    patch.patchBytes.push_back('E');
    patch.patchBytes.push_back('L');
    writeU32(patch.patchBytes, static_cast<uint32_t>(targetSize));

    if (baseSize == 0 || targetSize == 0) {
        if (targetSize > 0) {
            patch.patchBytes.push_back(OP_ADD);
            writeU32(patch.patchBytes, static_cast<uint32_t>(targetSize));
            patch.patchBytes.insert(patch.patchBytes.end(), target, target + targetSize);
        }
        return patch;
    }

    // Step 1: Index base chunk in 16-byte blocks
    constexpr size_t BLOCK_SIZE = 16;
    std::unordered_map<uint64_t, size_t> baseIndex;
    for (size_t i = 0; i + BLOCK_SIZE <= baseSize; i += BLOCK_SIZE) {
        uint64_t key;
        std::memcpy(&key, base + i, 8);
        baseIndex[key] = i;
    }

    // Step 2: Stream target chunk and emit COPY/ADD opcodes
    size_t targetPos = 0;
    std::vector<uint8_t> pendingAdd;

    auto flushPendingAdd = [&]() {
        if (!pendingAdd.empty()) {
            patch.patchBytes.push_back(OP_ADD);
            writeU32(patch.patchBytes, static_cast<uint32_t>(pendingAdd.size()));
            patch.patchBytes.insert(patch.patchBytes.end(), pendingAdd.begin(), pendingAdd.end());
            pendingAdd.clear();
        }
    };

    while (targetPos < targetSize) {
        bool matchFound = false;
        size_t bestBaseOffset = 0;
        size_t bestMatchLen = 0;

        if (targetPos + BLOCK_SIZE <= targetSize) {
            uint64_t key;
            std::memcpy(&key, target + targetPos, 8);
            auto it = baseIndex.find(key);
            if (it != baseIndex.end()) {
                size_t bPos = it->second;
                size_t tPos = targetPos;

                // Extend match forward using 64-bit word comparisons
                while (bPos + 8 <= baseSize && tPos + 8 <= targetSize) {
                    uint64_t wB, wT;
                    std::memcpy(&wB, base + bPos, 8);
                    std::memcpy(&wT, target + tPos, 8);
                    if (wB != wT) break;
                    bPos += 8;
                    tPos += 8;
                }
                while (bPos < baseSize && tPos < targetSize && base[bPos] == target[tPos]) {
                    bPos++;
                    tPos++;
                }

                size_t matchLen = tPos - targetPos;
                if (matchLen >= BLOCK_SIZE) {
                    matchFound = true;
                    bestBaseOffset = it->second;
                    bestMatchLen = matchLen;
                }
            }
        }

        if (matchFound) {
            flushPendingAdd();
            patch.patchBytes.push_back(OP_COPY);
            writeU32(patch.patchBytes, static_cast<uint32_t>(bestBaseOffset));
            writeU32(patch.patchBytes, static_cast<uint32_t>(bestMatchLen));
            targetPos += bestMatchLen;
        } else {
            pendingAdd.push_back(target[targetPos]);
            targetPos++;
        }
    }

    flushPendingAdd();

    if (targetSize > 0) {
        patch.compressionRatio = static_cast<double>(targetSize) / static_cast<double>(patch.patchBytes.size());
    }

    return patch;
}

std::vector<uint8_t> SIMDDeltaEngine::decodeDeltaSIMD(
    const uint8_t* base,
    size_t baseSize,
    const DeltaPatch& patch
) {
    if (patch.patchBytes.size() < 8) return {};
    if (std::memcmp(patch.patchBytes.data(), "GDEL", 4) != 0) return {};

    uint32_t targetSize = readU32(patch.patchBytes.data() + 4);
    std::vector<uint8_t> reconstructed(targetSize);

    size_t patchPos = 8;
    size_t outPos = 0;

    while (patchPos < patch.patchBytes.size() && outPos < targetSize) {
        uint8_t op = patch.patchBytes[patchPos++];
        if (op == OP_COPY) {
            if (patchPos + 8 > patch.patchBytes.size()) break;
            uint32_t baseOffset = readU32(patch.patchBytes.data() + patchPos); patchPos += 4;
            uint32_t length = readU32(patch.patchBytes.data() + patchPos); patchPos += 4;

            if (baseOffset + length <= baseSize && outPos + length <= targetSize) {
                std::memcpy(reconstructed.data() + outPos, base + baseOffset, length);
            }
            outPos += length;
        } else if (op == OP_ADD) {
            if (patchPos + 4 > patch.patchBytes.size()) break;
            uint32_t length = readU32(patch.patchBytes.data() + patchPos); patchPos += 4;

            if (patchPos + length <= patch.patchBytes.size() && outPos + length <= targetSize) {
                std::memcpy(reconstructed.data() + outPos, patch.patchBytes.data() + patchPos, length);
            }
            patchPos += length;
            outPos += length;
        }
    }

    return reconstructed;
}

} // namespace Core::Diffing
