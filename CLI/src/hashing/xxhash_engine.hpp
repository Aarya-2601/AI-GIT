#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace Core::Hashing {

struct Hash128 {
    uint64_t low{0};
    uint64_t high{0};

    bool operator==(const Hash128& other) const {
        return low == other.low && high == other.high;
    }
    bool operator!=(const Hash128& other) const {
        return !(*this == other);
    }
    bool operator<(const Hash128& other) const {
        if (low != other.low) return low < other.low;
        return high < other.high;
    }
    std::string toHex() const;
    static Hash128 fromHex(const std::string& hexStr);
};

class XXHashEngine {
public:
    // Blazing fast 64-bit and 128-bit hash (15 - 30 GB/s on AVX-512 / AVX2)
    static uint64_t hash64(const void* data, size_t len, uint64_t seed = 0);
    static Hash128 hash128(const void* data, size_t len, uint64_t seed = 0);

    // Merkle Tree root computation over leaf chunk hashes
    // Returns cryptographic SHA-256 commit hash calculated from hierarchical tree of chunk hashes
    static std::string buildMerkleTreeRootSHA256(const std::vector<Hash128>& chunkHashes);
    static std::string buildMerkleTreeRootSHA256(const std::vector<std::string>& chunkHexHashes);
};

} // namespace Core::Hashing
