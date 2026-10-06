#include "../core/mmap_file.hpp"
#include "../hashing/xxhash_engine.hpp"
#include "../chunking/tensor_fastcdc.hpp"
#include "../indexing/similarity_index.hpp"
#include "../diffing/simd_delta_engine.hpp"
#include "../compression/bitshuffle_zstd.hpp"
#include "../hashing/hashing.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

static std::string normalizePath(std::string p) {
    std::replace(p.begin(), p.end(), '\\', '/');
    return p;
}

void runBenchFile(const std::string& filePath) {
    Core::MMapFile mmap;
    auto t0 = std::chrono::high_resolution_clock::now();

    if (!mmap.open(filePath, true)) {
        std::cerr << "{\"error\": \"Could not mmap file: " << filePath << "\"}\n";
        return;
    }

    double initialRSS = Core::MMapFile::getCurrentProcessRSS_MB();
    size_t fSize = mmap.size();

    // 1. xxHash3-128 Throughput
    auto tHash0 = std::chrono::high_resolution_clock::now();
    auto hash = Core::Hashing::XXHashEngine::hash128(mmap.data(), fSize);
    auto tHash1 = std::chrono::high_resolution_clock::now();
    double hashSec = std::chrono::duration<double>(tHash1 - tHash0).count();
    double hashGBps = (fSize / (1024.0 * 1024.0 * 1024.0)) / (hashSec > 0 ? hashSec : 0.000001);

    // 2. Tensor-Aware FastCDC
    auto tChunk0 = std::chrono::high_resolution_clock::now();
    auto chunks = Core::Chunking::TensorAwareFastCDC::chunkMappedFile(mmap.data(), fSize, filePath);
    auto tChunk1 = std::chrono::high_resolution_clock::now();
    double chunkSec = std::chrono::duration<double>(tChunk1 - tChunk0).count();
    double chunkMBps = (fSize / (1024.0 * 1024.0)) / (chunkSec > 0 ? chunkSec : 0.000001);

    // 3. Merkle Tree Root
    std::vector<Core::Hashing::Hash128> leaves;
    for (const auto& c : chunks) leaves.push_back(c.xxhash128);
    std::string merkleRoot = Core::Hashing::XXHashEngine::buildMerkleTreeRootSHA256(leaves);

    // 4. Memory ceiling probe
    double peakRSS = Core::MMapFile::getCurrentProcessRSS_MB();

    std::cout << "{\n"
              << "  \"file\": \"" << normalizePath(filePath) << "\",\n"
              << "  \"file_size_bytes\": " << fSize << ",\n"
              << "  \"file_size_mb\": " << std::fixed << std::setprecision(2) << (fSize / (1024.0 * 1024.0)) << ",\n"
              << "  \"xxhash_throughput_gbps\": " << std::fixed << std::setprecision(2) << hashGBps << ",\n"
              << "  \"fastcdc_throughput_mbps\": " << std::fixed << std::setprecision(2) << chunkMBps << ",\n"
              << "  \"total_chunks\": " << chunks.size() << ",\n"
              << "  \"merkle_root_sha256\": \"" << merkleRoot << "\",\n"
              << "  \"initial_rss_mb\": " << std::fixed << std::setprecision(2) << initialRSS << ",\n"
              << "  \"peak_rss_mb\": " << std::fixed << std::setprecision(2) << peakRSS << ",\n"
              << "  \"memory_ceiling_safe\": " << (peakRSS < 1000.0 ? "true" : "false") << "\n"
              << "}\n";
}

void runBenchDelta(const std::string& basePath, const std::string& finetunedPath) {
    Core::MMapFile mmapBase;
    Core::MMapFile mmapFinetuned;

    if (!mmapBase.open(basePath) || !mmapFinetuned.open(finetunedPath)) {
        std::cerr << "{\"error\": \"Failed to open base or finetuned model\"}\n";
        return;
    }

    auto baseChunks = Core::Chunking::TensorAwareFastCDC::chunkMappedFile(mmapBase.data(), mmapBase.size(), basePath);
    auto fineChunks = Core::Chunking::TensorAwareFastCDC::chunkMappedFile(mmapFinetuned.data(), mmapFinetuned.size(), finetunedPath);

    Core::Indexing::SimilarityClusterIndex index;
    for (const auto& bc : baseChunks) {
        index.insertChunk(std::to_string(bc.chunkIndex), mmapBase.data() + bc.offset, bc.length);
    }

    size_t identicalChunks = 0;
    size_t deltaChunks = 0;
    size_t uniqueChunks = 0;
    size_t totalRawTargetBytes = mmapFinetuned.size();
    size_t totalCompressedDeltaBytes = 0;

    double reconVelocityTotalBytes = 0;
    double reconVelocityTotalSec = 0;

    for (const auto& fc : fineChunks) {
        const uint8_t* targetPtr = mmapFinetuned.data() + fc.offset;
        size_t targetLen = fc.length;

        // Check exact match first
        bool exactMatch = false;
        for (const auto& bc : baseChunks) {
            if (bc.xxhash128 == fc.xxhash128 && bc.length == fc.length) {
                exactMatch = true;
                break;
            }
        }

        if (exactMatch) {
            identicalChunks++;
            continue; // 0 bytes transferred
        }

        // LSH Similarity search
        auto match = index.queryBestMatch(targetPtr, targetLen, 0.60);
        if (match.found) {
            deltaChunks++;
            auto patch = Core::Diffing::SIMDDeltaEngine::encodeDeltaSIMD(
                match.baseData.data(), match.baseData.size(),
                targetPtr, targetLen, match.baseChunkId
            );
            totalCompressedDeltaBytes += patch.patchBytes.size();

            // Measure reconstruction velocity
            auto tR0 = std::chrono::high_resolution_clock::now();
            auto restored = Core::Diffing::SIMDDeltaEngine::decodeDeltaSIMD(
                match.baseData.data(), match.baseData.size(), patch
            );
            auto tR1 = std::chrono::high_resolution_clock::now();

            reconVelocityTotalBytes += restored.size();
            reconVelocityTotalSec += std::chrono::duration<double>(tR1 - tR0).count();
        } else {
            uniqueChunks++;
            auto comp = Core::Compression::BitshuffleCodec::compressFloatWeights(targetPtr, targetLen);
            totalCompressedDeltaBytes += comp.size();
        }
    }

    size_t bytesSaved = (totalRawTargetBytes > totalCompressedDeltaBytes) ? (totalRawTargetBytes - totalCompressedDeltaBytes) : 0;
    double dedupRatio = (static_cast<double>(bytesSaved) / static_cast<double>(totalRawTargetBytes)) * 100.0;
    double reconGBps = (reconVelocityTotalBytes / (1024.0 * 1024.0 * 1024.0)) / (reconVelocityTotalSec > 0 ? reconVelocityTotalSec : 0.000001);

    std::cout << "{\n"
              << "  \"base_file\": \"" << normalizePath(basePath) << "\",\n"
              << "  \"finetuned_file\": \"" << normalizePath(finetunedPath) << "\",\n"
              << "  \"base_size\": " << mmapBase.size() << ",\n"
              << "  \"finetuned_size\": " << totalRawTargetBytes << ",\n"
              << "  \"identical_chunks\": " << identicalChunks << ",\n"
              << "  \"simd_delta_chunks\": " << deltaChunks << ",\n"
              << "  \"unique_chunks\": " << uniqueChunks << ",\n"
              << "  \"bytes_saved_by_delta\": " << bytesSaved << ",\n"
              << "  \"deduplication_ratio\": " << std::fixed << std::setprecision(2) << dedupRatio << ",\n"
              << "  \"reconstruction_velocity_gbps\": " << std::fixed << std::setprecision(2) << reconGBps << ",\n"
              << "  \"peak_rss_mb\": " << std::fixed << std::setprecision(2) << Core::MMapFile::getCurrentProcessRSS_MB() << "\n"
              << "}\n";
}

void runBenchFolder(const std::string& folderPath) {
    auto tStart = std::chrono::high_resolution_clock::now();
    size_t totalFiles = 0;
    size_t totalBytes = 0;
    size_t totalChunks = 0;
    double maxRSS = 0.0;

    std::vector<Core::Hashing::Hash128> allLeafHashes;

    for (const auto& entry : fs::recursive_directory_iterator(folderPath, fs::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file()) continue;

        Core::MMapFile mmap;
        if (!mmap.open(entry.path(), true)) continue;

        size_t fSize = mmap.size();
        totalFiles++;
        totalBytes += fSize;

        if (fSize > 0) {
            auto chunks = Core::Chunking::TensorAwareFastCDC::chunkMappedFile(mmap.data(), fSize, entry.path());
            totalChunks += chunks.size();
            for (const auto& c : chunks) {
                allLeafHashes.push_back(c.xxhash128);
            }
        }

        double curRSS = Core::MMapFile::getCurrentProcessRSS_MB();
        if (curRSS > maxRSS) maxRSS = curRSS;
    }

    auto tEnd = std::chrono::high_resolution_clock::now();
    double totalSec = std::chrono::duration<double>(tEnd - tStart).count();
    double mbps = (totalBytes / (1024.0 * 1024.0)) / (totalSec > 0 ? totalSec : 0.000001);

    std::string rootMerkle = Core::Hashing::XXHashEngine::buildMerkleTreeRootSHA256(allLeafHashes);

    std::cout << "{\n"
              << "  \"benchmark\": \"RECURSIVE_FOLDER_AUDIT\",\n"
              << "  \"folder\": \"" << normalizePath(folderPath) << "\",\n"
              << "  \"total_files_processed\": " << totalFiles << ",\n"
              << "  \"total_size_bytes\": " << totalBytes << ",\n"
              << "  \"total_size_gb\": " << std::fixed << std::setprecision(2) << (totalBytes / (1024.0 * 1024.0 * 1024.0)) << ",\n"
              << "  \"total_chunks_indexed\": " << totalChunks << ",\n"
              << "  \"elapsed_seconds\": " << std::fixed << std::setprecision(2) << totalSec << ",\n"
              << "  \"sustained_throughput_mbps\": " << std::fixed << std::setprecision(2) << mbps << ",\n"
              << "  \"peak_physical_ram_mb\": " << std::fixed << std::setprecision(2) << maxRSS << ",\n"
              << "  \"memory_ceiling_safe_below_1gb\": " << (maxRSS < 1000.0 ? "true" : "false") << ",\n"
              << "  \"repo_root_merkle_sha256\": \"" << rootMerkle << "\"\n"
              << "}\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: aigit-engine <bench-file|bench-delta|bench-folder> [args...]\n";
        return 1;
    }

    std::string cmd = argv[1];
    if (cmd == "bench-file" && argc >= 3) {
        runBenchFile(argv[2]);
    } else if (cmd == "bench-delta" && argc >= 4) {
        runBenchDelta(argv[2], argv[3]);
    } else if (cmd == "bench-folder" && argc >= 3) {
        runBenchFolder(argv[2]);
    } else {
        std::cerr << "Invalid arguments\n";
        return 1;
    }
    return 0;
}
