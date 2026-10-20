#include "similarity_index.hpp"
#include <algorithm>
#include <cstring>

namespace Core::Indexing {

static const uint64_t HASH_SEEDS[MINHASH_SKETCH_SIZE] = {
    0x243f6a8885a308d3ULL, 0x13198a2e03707344ULL, 0xa4093822299f31d0ULL, 0x082efa98ec4e6c89ULL,
    0x452821e638d01377ULL, 0xbe5466cf34e90c6cULL, 0xc0ac29b7c97c50ddULL, 0x3f84d5b5b5470917ULL,
    0x9216d5d98979fb1bULL, 0xd1310ba698dfb5acULL, 0x2ffd72dbd01adfb7ULL, 0xb8e1afed6a267e96ULL,
    0xba7c9045f12c7f99ULL, 0x24a19947b3916cf7ULL, 0x0801f2e2858efc16ULL, 0x636920d871574e69ULL,
    0xa458fea3f4933d7eULL, 0x0d95748f728eb658ULL, 0x718bcd5882154aeeULL, 0x7b54a41dc25a59b5ULL,
    0x9c30d5392af26013ULL, 0xc5d1b023286085f0ULL, 0xca417918b8db38efULL, 0x8e79dcb0603a180eULL,
    0x6c9e0e8bb01e8a3eULL, 0xd71577c1bd7ffd61ULL, 0x3569d4c22d7f20cbULL, 0x9b90c33a25c6a1e5ULL,
    0x9212002167c13437ULL, 0x5a18a8b1fc6f452eULL, 0x9e6027a0db2bf50bULL, 0x74fc71bb0b7952dcULL,
    0x446e5b4b10b034fcULL, 0x7c7295175cf3a8d6ULL, 0x8eb9bcf1236fb64eULL, 0x5e2b02e77b4d45cfULL,
    0x8e30b329435b6999ULL, 0x6e2c4ef2db70c793ULL, 0x5990264101e4a132ULL, 0x815f9b4c44af509dULL,
    0x41f879ec19ef16ebULL, 0x19a9eb252d6a1334ULL, 0x1d5f308a0d7d3d19ULL, 0x13c8c7634f198b58ULL,
    0x92dfc97255152bb5ULL, 0x48981f21dd022f46ULL, 0x89ec936e788c7f99ULL, 0x2e861d8035172ea2ULL,
    0x90f671c67d344ec7ULL, 0x879893d586146c8eULL, 0x8dfb3b890a20a40fULL, 0x7e8688402778bb82ULL,
    0x6cdfd47a0665f80bULL, 0x05eb1432f8217bb4ULL, 0x3cbcf79a45638c4bULL, 0x16b0b3706050bc55ULL,
    0x29ef9f75a74d284aULL, 0x22be4824317b35f2ULL, 0xd03e1e072b217086ULL, 0x77c27d42cfc05ab2ULL,
    0xa0670984a9217641ULL, 0x08a902123547ec94ULL, 0x429671d17cf36657ULL, 0x2984fb1e687cfba1ULL
};

static inline uint64_t hash64_seed(uint64_t val, uint64_t seed) {
    val ^= seed;
    val ^= val >> 33;
    val *= 0xff51afd7ed558ccdULL;
    val ^= val >> 33;
    val *= 0xc4ceb9fe1a85ec53ULL;
    val ^= val >> 33;
    return val;
}

MinHashSketch SimilarityClusterIndex::computeSketch(const uint8_t* data, size_t size) {
    MinHashSketch sketch;
    for (size_t i = 0; i < MINHASH_SKETCH_SIZE; ++i) {
        sketch.values[i] = UINT64_MAX;
    }
    if (!data || size < 8) return sketch;

    // Rolling 8-byte shingles with fixed stride for size-invariant alignment
    const size_t step = (size <= 256) ? 8 : 32;
    for (size_t offset = 0; offset + 8 <= size; offset += step) {
        uint64_t shingle;
        std::memcpy(&shingle, data + offset, 8);

        for (size_t i = 0; i < MINHASH_SKETCH_SIZE; ++i) {
            uint64_t h = hash64_seed(shingle, HASH_SEEDS[i]);
            if (h < sketch.values[i]) {
                sketch.values[i] = h;
            }
        }
    }
    return sketch;
}

SimilarityMatch SimilarityClusterIndex::queryBestMatch(const uint8_t* data, size_t size, double threshold) {
    SimilarityMatch best;
    if (entries_.empty()) return best;

    MinHashSketch querySketch = computeSketch(data, size);
    double maxSim = 0.0;
    size_t bestIdx = 0;

    for (size_t i = 0; i < entries_.size(); ++i) {
        double sim = querySketch.computeJaccardSimilarity(entries_[i].sketch);
        if (sim > maxSim) {
            maxSim = sim;
            bestIdx = i;
        }
    }

    if (maxSim >= threshold) {
        best.found = true;
        best.baseChunkId = entries_[bestIdx].chunkId;
        best.similarity = maxSim;
        best.baseChunkSize = entries_[bestIdx].data.size();
        best.baseData = entries_[bestIdx].data;
    }
    return best;
}

void SimilarityClusterIndex::insertChunk(const std::string& chunkId, const uint8_t* data, size_t size) {
    IndexEntry entry;
    entry.chunkId = chunkId;
    entry.sketch = computeSketch(data, size);
    entry.data.assign(data, data + size);
    entries_.push_back(std::move(entry));
}

} // namespace Core::Indexing
