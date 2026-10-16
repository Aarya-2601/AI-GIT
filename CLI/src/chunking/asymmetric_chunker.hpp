#pragma once

#include "../inspection/file_inspector.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Core
{

enum class ChunkCategory
{
    MetadataHeader,  // Volatile metadata (SafeTensors JSON header, Parquet header/footer)
    TensorPayload,   // Neural network weights (large chunks, high deduplication)
    DatasetPayload,  // Columnar dataset data blocks (Parquet row-groups)
    MediaPayload,    // Pre-compressed media stream (video/images)
    GenericData      // Standard FastCDC content
};

struct StructuralChunk
{
    size_t offset;
    size_t length;
    std::string sha256;
    ChunkCategory category = ChunkCategory::GenericData;
    std::string processingMethod; // Explicit name of chunking method for this file type
};

struct ChunkerProfile
{
    size_t minSize;
    size_t avgSize;
    size_t maxSize;
};

class AsymmetricChunker
{
public:
    // Profiles for asymmetric sizing
    static constexpr ChunkerProfile HEADER_PROFILE = {
        8 * 1024,      // 8 KB min
        32 * 1024,     // 32 KB avg
        64 * 1024      // 64 KB max
    };

    static constexpr ChunkerProfile TENSOR_PROFILE = {
        2 * 1024 * 1024,   // 2 MB min
        8 * 1024 * 1024,   // 8 MB avg
        16 * 1024 * 1024   // 16 MB max
    };

    static constexpr ChunkerProfile GENERIC_PROFILE = {
        256 * 1024,        // 256 KB min
        1024 * 1024,       // 1 MB avg
        4 * 1024 * 1024    // 4 MB max
    };

    // Chunks an entire file with boundary alignment informed by inspection
    static std::vector<StructuralChunk> chunkFile(
        const std::filesystem::path& filePath,
        const InspectionResult& inspection
    );

    // Chunks an in-memory buffer with boundary alignment
    static std::vector<StructuralChunk> chunkBuffer(
        const uint8_t* data,
        size_t size,
        const InspectionResult& inspection
    );

    // Explicit format-specific chunking methods
    static std::vector<StructuralChunk> chunkSafeTensorsWithBoundarySplit(
        const uint8_t* data,
        size_t size,
        const InspectionResult& inspection
    );

    static std::vector<StructuralChunk> chunkParquetWithStructuralSlices(
        const uint8_t* data,
        size_t size,
        const InspectionResult& inspection
    );

    static std::vector<StructuralChunk> chunkMediaAsAtomicBlob(
        const uint8_t* data,
        size_t size,
        const InspectionResult& inspection
    );

    static std::vector<StructuralChunk> chunkGenericWithFastCDC(
        const uint8_t* data,
        size_t size,
        const InspectionResult& inspection
    );

    // Slices a specific buffer region with customized FastCDC parameters
    static std::vector<StructuralChunk> chunkSlice(
        const uint8_t* data,
        size_t size,
        size_t baseOffset,
        ChunkCategory category,
        const ChunkerProfile& profile,
        const std::string& methodName = "chunkSlice"
    );

    static const char* categoryToString(ChunkCategory cat);
};

} // namespace Core
