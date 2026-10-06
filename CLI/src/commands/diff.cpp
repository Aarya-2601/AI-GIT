#include "diff.hpp"
#include "../core/mmap_file.hpp"
#include "../hashing/hashing.hpp"
#include "../inspection/file_inspector.hpp"
#include "../diffing/structural_differ.hpp"
#include "../diffing/metadata_harvester.hpp"
#include "../diffing/simd_delta_engine.hpp"
#include "../indexing/similarity_index.hpp"
#include "../chunking/tensor_fastcdc.hpp"
#include "../compression/bitshuffle_zstd.hpp"
#include "../core/index.hpp"
#include "../core/storage.hpp"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

namespace Commands {

int runInspect(const std::string& filePath) {
    if (!fs::exists(filePath)) {
        std::cerr << "Error: File does not exist: " << filePath << std::endl;
        return 1;
    }

    Core::MMapFile mmap;
    if (!mmap.open(filePath, true)) {
        std::cerr << "Error: Could not memory-map file: " << filePath << std::endl;
        return 1;
    }

    fs::path p(filePath);
    auto insp = Core::FileInspector::inspectBuffer(mmap.data(), std::min(mmap.size(), size_t(64)), mmap.size(), &p);

    std::cout << "\n=======================================================\n";
    std::cout << "  AI-GIT FORMAT INSPECTION: " << filePath << "\n";
    std::cout << "=======================================================\n";
    std::cout << "  Format:         " << Core::FileInspector::formatToString(insp.format) << "\n";
    std::cout << "  Category:       " << Core::FileInspector::categoryToString(insp.category) << "\n";
    std::cout << "  MIME Type:      " << insp.mimeType << "\n";
    std::cout << "  Total Size:     " << std::fixed << std::setprecision(2) << (mmap.size() / (1024.0 * 1024.0)) << " MB (" << mmap.size() << " bytes)\n";
    std::cout << "  Payload Offset: " << insp.payloadOffset << " bytes\n";

    if (insp.format == Core::FileFormat::SafeTensors) {
        Core::ModelMetadata meta;
        Core::MetadataHarvester::harvestSafeTensorsMetadata(p, meta);
        std::cout << "  Total Tensors:  " << meta.tensors.size() << "\n";
        std::cout << "-------------------------------------------------------\n";
        std::cout << "  Sample Tensor Layers:\n";
        size_t showCount = (meta.tensors.size() < 8) ? meta.tensors.size() : 8;
        for (size_t i = 0; i < showCount; ++i) {
            std::cout << "    [" << (i + 1) << "] " << meta.tensors[i].name
                      << " | " << meta.tensors[i].dtype << " | [";
            for (size_t d = 0; d < meta.tensors[i].shape.size(); ++d) {
                std::cout << meta.tensors[i].shape[d] << (d + 1 < meta.tensors[i].shape.size() ? ", " : "");
            }
            std::cout << "] (" << meta.tensors[i].byteLength << " bytes)\n";
        }
        if (meta.tensors.size() > showCount) {
            std::cout << "    ... and " << (meta.tensors.size() - showCount) << " more tensors.\n";
        }
    }
    std::cout << "=======================================================\n\n";
    return 0;
}

int runDiff(const std::vector<std::string>& targets) {
    if (targets.size() < 2) {
        std::cerr << "Usage: ai-git diff <fileA> <fileB>" << std::endl;
        return 1;
    }

    std::string pathA = targets[0];
    std::string pathB = targets[1];

    if (!fs::exists(pathA)) {
        std::cerr << "Error: File A does not exist: " << pathA << std::endl;
        return 1;
    }
    if (!fs::exists(pathB)) {
        std::cerr << "Error: File B does not exist: " << pathB << std::endl;
        return 1;
    }

    Core::MMapFile mmapA, mmapB;
    if (!mmapA.open(pathA, true) || !mmapB.open(pathB, true)) {
        std::cerr << "Error: Could not mmap files for diffing." << std::endl;
        return 1;
    }

    fs::path pA(pathA);
    fs::path pB(pathB);
    auto inspA = Core::FileInspector::inspectBuffer(mmapA.data(), std::min(mmapA.size(), size_t(64)), mmapA.size(), &pA);
    auto inspB = Core::FileInspector::inspectBuffer(mmapB.data(), std::min(mmapB.size(), size_t(64)), mmapB.size(), &pB);

    std::cout << "\n=======================================================\n";
    std::cout << "  AI-GIT SEMANTIC & SIMD DELTA DIFF\n";
    std::cout << "  File A: " << pathA << " (" << std::fixed << std::setprecision(2) << (mmapA.size() / (1024.0 * 1024.0)) << " MB)\n";
    std::cout << "  File B: " << pathB << " (" << std::fixed << std::setprecision(2) << (mmapB.size() / (1024.0 * 1024.0)) << " MB)\n";
    std::cout << "=======================================================\n";

    // 1. SafeTensors Model Diff
    if (inspA.format == Core::FileFormat::SafeTensors && inspB.format == Core::FileFormat::SafeTensors) {
        auto semDiff = Core::StructuralDiffer::diffFiles(pA, pB);
        if (semDiff.hasModelDiff) {
            std::cout << "  [STRUCTURAL MODEL COMPARISON]\n";
            std::cout << "    • Unchanged Layers: " << semDiff.modelDiff.identicalLayers << "\n";
            std::cout << "    • Modified Layers:  " << semDiff.modelDiff.modifiedLayers << "\n";
            std::cout << "    • Added Layers:     " << semDiff.modelDiff.addedLayers << "\n";
            std::cout << "    • Removed Layers:   " << semDiff.modelDiff.removedLayers << "\n\n";
        }

        // 2. SIMD Binary Delta Benchmark on Chunks
        auto baseChunks = Core::Chunking::TensorAwareFastCDC::chunkMappedFile(mmapA.data(), mmapA.size(), pathA);
        auto fineChunks = Core::Chunking::TensorAwareFastCDC::chunkMappedFile(mmapB.data(), mmapB.size(), pathB);

        Core::Indexing::SimilarityClusterIndex index;
        for (const auto& bc : baseChunks) {
            index.insertChunk(std::to_string(bc.chunkIndex), mmapA.data() + bc.offset, bc.length);
        }

        size_t identicalChunks = 0;
        size_t deltaChunks = 0;
        size_t uniqueChunks = 0;
        size_t totalRawTargetBytes = mmapB.size();
        size_t totalCompressedDeltaBytes = 0;

        double reconVelocityTotalBytes = 0;
        double reconVelocityTotalSec = 0;

        for (const auto& fc : fineChunks) {
            const uint8_t* targetPtr = mmapB.data() + fc.offset;
            size_t targetLen = fc.length;

            bool exactMatch = false;
            for (const auto& bc : baseChunks) {
                if (bc.xxhash128 == fc.xxhash128 && bc.length == fc.length) {
                    exactMatch = true;
                    break;
                }
            }

            if (exactMatch) {
                identicalChunks++;
                continue;
            }

            auto match = index.queryBestMatch(targetPtr, targetLen, 0.60);
            if (match.found) {
                deltaChunks++;
                auto patch = Core::Diffing::SIMDDeltaEngine::encodeDeltaSIMD(
                    match.baseData.data(), match.baseData.size(),
                    targetPtr, targetLen, match.baseChunkId
                );
                totalCompressedDeltaBytes += patch.patchBytes.size();

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
        double dedupRatio = totalRawTargetBytes > 0 ? (static_cast<double>(bytesSaved) / static_cast<double>(totalRawTargetBytes) * 100.0) : 0.0;
        double reconVelocityGBps = (reconVelocityTotalSec > 0) ? ((reconVelocityTotalBytes / (1024.0 * 1024.0 * 1024.0)) / reconVelocityTotalSec) : 0.0;

        std::cout << "  [SIMD BINARY DELTA SAVINGS]\n";
        std::cout << "    • Identical Chunks:        " << identicalChunks << "\n";
        std::cout << "    • SIMD Delta Chunks:       " << deltaChunks << "\n";
        std::cout << "    • Unique Chunks:           " << uniqueChunks << "\n";
        std::cout << "    • Deduplication Ratio:     " << std::fixed << std::setprecision(2) << dedupRatio << "%\n";
        std::cout << "    • Bytes Saved by Delta:    " << (bytesSaved / (1024.0 * 1024.0)) << " MB\n";
        std::cout << "    • Reconstruction Velocity: " << std::fixed << std::setprecision(2) << reconVelocityGBps << " GB/s\n";
    } else {
        std::cout << "  Both files compared using Content-Addressed cryptographic comparison.\n";
        std::cout << "  File A Hash: " << Core::calcSHA256(std::string(reinterpret_cast<const char*>(mmapA.data()), mmapA.size())) << "\n";
        std::cout << "  File B Hash: " << Core::calcSHA256(std::string(reinterpret_cast<const char*>(mmapB.data()), mmapB.size())) << "\n";
    }

    std::cout << "=======================================================\n\n";
    return 0;
}

} // namespace Commands
