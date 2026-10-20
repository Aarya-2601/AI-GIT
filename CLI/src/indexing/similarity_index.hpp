#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <cstddef>

namespace Core::Indexing {

constexpr size_t MINHASH_SKETCH_SIZE = 64;

struct MinHashSketch {
    uint64_t values[MINHASH_SKETCH_SIZE];

    double computeJaccardSimilarity(const MinHashSketch& other) const {
        size_t matches = 0;
        for (size_t i = 0; i < MINHASH_SKETCH_SIZE; ++i) {
            if (values[i] == other.values[i]) matches++;
        }
        return static_cast<double>(matches) / static_cast<double>(MINHASH_SKETCH_SIZE);
    }
};

struct SimilarityMatch {
    bool found{false};
    std::string baseChunkId;
    double similarity{0.0};
    size_t baseChunkSize{0};
    std::vector<uint8_t> baseData;
};

class SimilarityClusterIndex {
public:
    SimilarityClusterIndex() = default;

    // Computes 64-permutation MinHash sketch on chunk bytes
    static MinHashSketch computeSketch(const uint8_t* data, size_t size);

    // Queries index: returns best base chunk match if Jaccard similarity >= threshold
    SimilarityMatch queryBestMatch(const uint8_t* data, size_t size, double threshold = 0.85);

    // Inserts base chunk into LSH cluster index
    void insertChunk(const std::string& chunkId, const uint8_t* data, size_t size);

    size_t size() const { return entries_.size(); }

private:
    struct IndexEntry {
        std::string chunkId;
        MinHashSketch sketch;
        std::vector<uint8_t> data;
    };

    std::vector<IndexEntry> entries_;
    std::unordered_map<uint64_t, std::vector<size_t>> lshBuckets_;
};

} // namespace Core::Indexing
