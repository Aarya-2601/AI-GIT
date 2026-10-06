#include "xxhash_engine.hpp"
#include "hashing.hpp"
#include <iomanip>
#include <sstream>
#include <cstring>

namespace Core::Hashing {

static constexpr uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;
static constexpr uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
static constexpr uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;
static constexpr uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
static constexpr uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;

static inline uint64_t rotl64(uint64_t x, int r) {
    return (x << r) | (x >> (64 - r));
}

static inline uint64_t round64(uint64_t acc, uint64_t input) {
    acc += input * PRIME64_2;
    acc = rotl64(acc, 31);
    acc *= PRIME64_1;
    return acc;
}

static inline uint64_t read64LE(const uint8_t* p) {
    uint64_t v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

std::string Hash128::toHex() const {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(16) << high
       << std::setw(16) << low;
    return ss.str();
}

Hash128 Hash128::fromHex(const std::string& hexStr) {
    Hash128 h;
    if (hexStr.size() >= 32) {
        std::string highStr = hexStr.substr(0, 16);
        std::string lowStr = hexStr.substr(16, 16);
        h.high = std::stoull(highStr, nullptr, 16);
        h.low  = std::stoull(lowStr, nullptr, 16);
    }
    return h;
}

uint64_t XXHashEngine::hash64(const void* data, size_t len, uint64_t seed) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    const uint8_t* bEnd = p + len;
    uint64_t h64;

    if (len >= 32) {
        const uint8_t* const limit = bEnd - 32;
        uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
        uint64_t v2 = seed + PRIME64_2;
        uint64_t v3 = seed + 0;
        uint64_t v4 = seed - PRIME64_1;

        do {
            v1 = round64(v1, read64LE(p)); p += 8;
            v2 = round64(v2, read64LE(p)); p += 8;
            v3 = round64(v3, read64LE(p)); p += 8;
            v4 = round64(v4, read64LE(p)); p += 8;
        } while (p <= limit);

        h64 = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);
        v1 = round64(0, v1); h64 ^= v1; h64 = h64 * PRIME64_1 + PRIME64_4;
        v2 = round64(0, v2); h64 ^= v2; h64 = h64 * PRIME64_1 + PRIME64_4;
        v3 = round64(0, v3); h64 ^= v3; h64 = h64 * PRIME64_1 + PRIME64_4;
        v4 = round64(0, v4); h64 ^= v4; h64 = h64 * PRIME64_1 + PRIME64_4;
    } else {
        h64 = seed + PRIME64_5;
    }

    h64 += static_cast<uint64_t>(len);

    while (p + 8 <= bEnd) {
        uint64_t k1 = round64(0, read64LE(p));
        h64 ^= k1;
        h64 = rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
        p += 8;
    }

    if (p + 4 <= bEnd) {
        uint32_t k2;
        std::memcpy(&k2, p, 4);
        h64 ^= static_cast<uint64_t>(k2) * PRIME64_1;
        h64 = rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
        p += 4;
    }

    while (p < bEnd) {
        h64 ^= (*p) * PRIME64_5;
        h64 = rotl64(h64, 11) * PRIME64_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;
}

Hash128 XXHashEngine::hash128(const void* data, size_t len, uint64_t seed) {
    Hash128 result;
    result.low = hash64(data, len, seed);
    result.high = hash64(data, len, seed ^ PRIME64_1);
    return result;
}

std::string XXHashEngine::buildMerkleTreeRootSHA256(const std::vector<Hash128>& chunkHashes) {
    if (chunkHashes.empty()) {
        return Core::calcSHA256("");
    }

    std::vector<std::string> currentLevel;
    currentLevel.reserve(chunkHashes.size());
    for (const auto& h : chunkHashes) {
        currentLevel.push_back(h.toHex());
    }

    return buildMerkleTreeRootSHA256(currentLevel);
}

std::string XXHashEngine::buildMerkleTreeRootSHA256(const std::vector<std::string>& chunkHexHashes) {
    if (chunkHexHashes.empty()) {
        return Core::calcSHA256("");
    }
    if (chunkHexHashes.size() == 1) {
        return Core::calcSHA256(chunkHexHashes[0]);
    }

    std::vector<std::string> currentLevel = chunkHexHashes;

    while (currentLevel.size() > 1) {
        std::vector<std::string> nextLevel;
        nextLevel.reserve((currentLevel.size() + 1) / 2);

        for (size_t i = 0; i < currentLevel.size(); i += 2) {
            if (i + 1 < currentLevel.size()) {
                std::string combined = currentLevel[i] + currentLevel[i + 1];
                nextLevel.push_back(Core::calcSHA256(combined));
            } else {
                // Odd element promoted to next level
                nextLevel.push_back(currentLevel[i]);
            }
        }
        currentLevel = std::move(nextLevel);
    }

    return currentLevel[0];
}

} // namespace Core::Hashing
