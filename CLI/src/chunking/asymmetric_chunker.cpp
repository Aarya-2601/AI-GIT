#include "asymmetric_chunker.hpp"
#include "../hashing/hashing.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace Core
{

// 256 64-bit random values for the Gear hashing matrix (FastCDC algorithm)
static const uint64_t GEAR_MATRIX[256] = {
    0x3565a0ec7b80a563ULL, 0x1f0b094602f7ff60ULL, 0xa1bf785420352ef2ULL, 0xd008dd52f75a6c38ULL,
    0x2c68e4209930f555ULL, 0x4896eec50e685f0aULL, 0x6e9f783ef2e987c9ULL, 0x93309a473f1d8c11ULL,
    0x442dfa8f108849adULL, 0xa8c49e7b233a5987ULL, 0xcb953a7629b35bc4ULL, 0xd9b7754668b57732ULL,
    0x67db2b56e6d338f0ULL, 0x92f9d51a660a92d7ULL, 0x199fa688f1521908ULL, 0x51c36b4474776b38ULL,
    0x8e8a93e2dc11cf9aULL, 0x53503db458efda6eULL, 0xb8beec072c4ec096ULL, 0x446d3211e4e69d72ULL,
    0x88981f440b82f64dULL, 0x2287955c2763f0aeULL, 0xdf8435d8cfd98caeULL, 0x9a805f6cb1cfd8c7ULL,
    0x88498f48b12f6a7cULL, 0x1d9539268f7b5a88ULL, 0x4e6b2c68e4209930ULL, 0x2649a8c49e7b233aULL,
    0x92e5cb953a7629b3ULL, 0x76b5d9b7754668b5ULL, 0x82f967db2b56e6d3ULL, 0x56a692f9d51a660aULL,
    0x4811199fa688f152ULL, 0x9f5151c36b447477ULL, 0xb0e88e8a93e2dc11ULL, 0xf05353503db458efULL,
    0xc6b8b8beec072c4eULL, 0x9844446d3211e4e6ULL, 0x828888981f440b82ULL, 0x63222287955c2763ULL,
    0xd9dfdf8435d8cfd9ULL, 0xcb9a9a805f6cb1cfULL, 0x2f8888498f48b12fULL, 0x7b1d1d9539268f7bULL,
    0x204e4e6b2c68e420ULL, 0x7b262649a8c49e7bULL, 0x769292e5cb953a76ULL, 0x467676b5d9b77546ULL,
    0x568282f967db2b56ULL, 0x1a5656a692f9d51aULL, 0x88484811199fa688ULL, 0x449f9f5151c36b44ULL,
    0xe2b0b0e88e8a93e2ULL, 0xb4f0f05353503db4ULL, 0x07c6c6b8b8beec07ULL, 0x11989844446d3211ULL,
    0x4482828888981f44ULL, 0x5c6363222287955cULL, 0xd8d9d9dfdf8435d8ULL, 0x6ccb9a9a805f6cb1ULL,
    0x8f2f88888498f48bULL, 0x397b1d1d9539268fULL, 0x68204e4e6b2c68e4ULL, 0x9e7b262649a8c49eULL,
    0x3a769292e5cb953aULL, 0x75467676b5d9b775ULL, 0x2b568282f967db2bULL, 0xd51a5656a692f9d5ULL,
    0xa688484811199fa6ULL, 0x6b449f9f5151c36bULL, 0x93e2b0b0e88e8a93ULL, 0x3db4f0f05353503dULL,
    0xec07c6c6b8b8beecULL, 0x3211989844446d32ULL, 0x1f4482828888981fULL, 0x955c636322228795ULL,
    0x35d8d9d9df8435d8ULL, 0x5f6ccb9a9a805f6cULL, 0x8f48b12f6a7c8849ULL, 0x39268f7b5a881d95ULL,
    0x2c68e42099304e6bULL, 0x9e7b233a2649a8c4ULL, 0x3a7629b392e5cb95ULL, 0x754668b576b5d9b7ULL,
    0x2b56e6d382f967dbULL, 0xd51a660a56a692f9ULL, 0xa688f1524811199fULL, 0x6b4474779f5151c3ULL,
    0x93e2dc11b0e88e8aULL, 0x3db458eff0535350ULL, 0xec072c4ec6b8b8beULL, 0x3211e4e69844446dULL,
    0x1f440b8282888898ULL, 0x955c276363222287ULL, 0x35d8cfd9d9dfdf84ULL, 0x5f6cb1cfcb9a9a80ULL,
    0x8f48b12f8888498fULL, 0x39268f7b1d1d9539ULL, 0x2c68e4204e4e6b2cULL, 0x9e7b233a7b262649ULL,
    0x3a7629b3769292e5ULL, 0x754668b5467676b5ULL, 0x2b56e6d3568282f9ULL, 0xd51a660a1a5656a6ULL,
    0xa688f15288484811ULL, 0x6b447477449f9f51ULL, 0x93e2dc11e2b0b0e8ULL, 0x3db458efb4f0f053ULL,
    0xec072c4e07c6c6b8ULL, 0x3211e4e611989844ULL, 0x1f440b8244828288ULL, 0x955c27635c636322ULL,
    0x35d8cfd9d8d9d9dfULL, 0x5f6cb1cf6ccb9a9aULL, 0x88498f48b12f6a7cULL, 0x1d9539268f7b5a88ULL,
    0x4e6b2c68e4209930ULL, 0x2649a8c49e7b233aULL, 0x92e5cb953a7629b3ULL, 0x76b5d9b7754668b5ULL,
    0x82f967db2b56e6d3ULL, 0x56a692f9d51a660aULL, 0x4811199fa688f152ULL, 0x9f5151c36b447477ULL,
    0xb0e88e8a93e2dc11ULL, 0xf05353503db458efULL, 0xc6b8b8beec072c4eULL, 0x9844446d3211e4e6ULL,
    0x828888981f440b82ULL, 0x63222287955c2763ULL, 0xd9dfdf8435d8cfd9ULL, 0xcb9a9a805f6cb1cfULL,
    0x2f8888498f48b12fULL, 0x7b1d1d9539268f7bULL, 0x204e4e6b2c68e420ULL, 0x7b262649a8c49e7bULL,
    0x769292e5cb953a76ULL, 0x467676b5d9b77546ULL, 0x568282f967db2b56ULL, 0x1a5656a692f9d51aULL,
    0x88484811199fa688ULL, 0x449f9f5151c36b44ULL, 0xe2b0b0e88e8a93e2ULL, 0xb4f0f05353503db4ULL,
    0x07c6c6b8b8beec07ULL, 0x11989844446d3211ULL, 0x4482828888981f44ULL, 0x5c6363222287955cULL,
    0xd8d9d9dfdf8435d8ULL, 0x6ccb9a9a805f6cb1ULL, 0x8f2f88888498f48bULL, 0x397b1d1d9539268fULL,
    0x68204e4e6b2c68e4ULL, 0x9e7b262649a8c49eULL, 0x3a769292e5cb953aULL, 0x75467676b5d9b775ULL,
    0x2b568282f967db2bULL, 0xd51a5656a692f9d5ULL, 0xa688484811199fa6ULL, 0x6b449f9f5151c36bULL,
    0x93e2b0b0e88e8a93ULL, 0x3db4f0f05353503dULL, 0xec07c6c6b8b8beecULL, 0x3211989844446d32ULL,
    0x1f4482828888981fULL, 0x955c636322228795ULL, 0x35d8d9d9df8435d8ULL, 0x5f6ccb9a9a805f6cULL,
    0x8f48b12f6a7c8849ULL, 0x39268f7b5a881d95ULL, 0x2c68e42099304e6bULL, 0x9e7b233a2649a8c4ULL,
    0x3a7629b392e5cb95ULL, 0x754668b576b5d9b7ULL, 0x2b56e6d382f967dbULL, 0xd51a660a56a692f9ULL,
    0xa688f1524811199fULL, 0x6b4474779f5151c3ULL, 0x93e2dc11b0e88e8aULL, 0x3db458eff0535350ULL,
    0xec072c4ec6b8b8beULL, 0x3211e4e69844446dULL, 0x1f440b8282888898ULL, 0x955c276363222287ULL,
    0x35d8cfd9d9dfdf84ULL, 0x5f6cb1cfcb9a9a80ULL, 0x8f48b12f8888498fULL, 0x39268f7b1d1d9539ULL,
    0x2c68e4204e4e6b2cULL, 0x9e7b233a7b262649ULL, 0x3a7629b3769292e5ULL, 0x754668b5467676b5ULL,
    0x2b56e6d3568282f9ULL, 0xd51a660a1a5656a6ULL, 0xa688f15288484811ULL, 0x6b447477449f9f51ULL,
    0x93e2dc11e2b0b0e8ULL, 0x3db458efb4f0f053ULL, 0xec072c4e07c6c6b8ULL, 0x3211e4e611989844ULL,
    0x1f440b8244828288ULL, 0x955c27635c636322ULL, 0x35d8cfd9d8d9d9dfULL, 0x5f6cb1cf6ccb9a9aULL,
    0x88498f48b12f6a7cULL, 0x1d9539268f7b5a88ULL, 0x4e6b2c68e4209930ULL, 0x2649a8c49e7b233aULL,
    0x92e5cb953a7629b3ULL, 0x76b5d9b7754668b5ULL, 0x82f967db2b56e6d3ULL, 0x56a692f9d51a660aULL,
    0x4811199fa688f152ULL, 0x9f5151c36b447477ULL, 0xb0e88e8a93e2dc11ULL, 0xf05353503db458efULL,
    0xc6b8b8beec072c4eULL, 0x9844446d3211e4e6ULL, 0x828888981f440b82ULL, 0x63222287955c2763ULL,
    0xd9dfdf8435d8cfd9ULL, 0xcb9a9a805f6cb1cfULL, 0x2f8888498f48b12fULL, 0x7b1d1d9539268f7bULL,
    0x204e4e6b2c68e420ULL, 0x7b262649a8c49e7bULL, 0x769292e5cb953a76ULL, 0x467676b5d9b77546ULL,
    0x568282f967db2b56ULL, 0x1a5656a692f9d51aULL, 0x88484811199fa688ULL, 0x449f9f5151c36b44ULL,
    0xe2b0b0e88e8a93e2ULL, 0xb4f0f05353503db4ULL, 0x07c6c6b8b8beec07ULL, 0x11989844446d3211ULL,
    0x4482828888981f44ULL, 0x5c6363222287955cULL, 0xd8d9d9dfdf8435d8ULL, 0x6ccb9a9a805f6cb1ULL
};

const char* AsymmetricChunker::categoryToString(ChunkCategory cat)
{
    switch (cat)
    {
        case ChunkCategory::MetadataHeader: return "MetadataHeader";
        case ChunkCategory::TensorPayload:  return "TensorPayload";
        case ChunkCategory::DatasetPayload: return "DatasetPayload";
        case ChunkCategory::MediaPayload:   return "MediaPayload";
        case ChunkCategory::GenericData:    return "GenericData";
        default:                            return "GenericData";
    }
}

std::vector<StructuralChunk> AsymmetricChunker::chunkSlice(
    const uint8_t* data,
    size_t size,
    size_t baseOffset,
    ChunkCategory category,
    const ChunkerProfile& profile,
    const std::string& methodName
)
{
    std::vector<StructuralChunk> chunks;
    if (data == nullptr || size == 0)
    {
        return chunks;
    }

    if (size <= profile.minSize)
    {
        std::string chunkStr(reinterpret_cast<const char*>(data), size);
        chunks.push_back({
            baseOffset,
            size,
            Core::calcSHA256(chunkStr),
            category,
            methodName
        });
        return chunks;
    }

    int avgBits = static_cast<int>(std::round(std::log2(static_cast<double>(profile.avgSize))));
    if (avgBits < 10) avgBits = 10;
    if (avgBits > 28) avgBits = 28;

    int maskBits = avgBits - 1;
    uint64_t mask = (1ULL << maskBits) - 1ULL;

    size_t offset = 0;
    while (offset < size)
    {
        size_t remaining = size - offset;
        if (remaining <= profile.minSize)
        {
            std::string chunkStr(reinterpret_cast<const char*>(data + offset), remaining);
            chunks.push_back({
                baseOffset + offset,
                remaining,
                Core::calcSHA256(chunkStr),
                category,
                methodName
            });
            break;
        }

        size_t scanStart = offset + profile.minSize;
        size_t scanMax = offset + std::min(remaining, profile.maxSize);
        size_t cutPoint = scanMax;

        uint64_t fp = 0;
        for (size_t i = scanStart; i < scanMax; ++i)
        {
            fp = (fp << 1) + GEAR_MATRIX[data[i]];
            if ((fp & mask) == 0)
            {
                cutPoint = i + 1;
                break;
            }
        }

        size_t chunkLen = cutPoint - offset;
        std::string chunkStr(reinterpret_cast<const char*>(data + offset), chunkLen);
        chunks.push_back({
            baseOffset + offset,
            chunkLen,
            Core::calcSHA256(chunkStr),
            category,
            methodName
        });

        offset += chunkLen;
    }

    return chunks;
}

// ----------------------------------------------------------------------------
// Explicit File-Type Chunking Methods
// ----------------------------------------------------------------------------

std::vector<StructuralChunk> AsymmetricChunker::chunkSafeTensorsWithBoundarySplit(
    const uint8_t* data,
    size_t size,
    const InspectionResult& inspection
)
{
    std::vector<StructuralChunk> chunks;
    size_t headerSize = static_cast<size_t>(inspection.payloadOffset);

    // 1. Header Slice (Volatile JSON metadata)
    auto headerChunks = chunkSlice(
        data,
        headerSize,
        0,
        ChunkCategory::MetadataHeader,
        HEADER_PROFILE,
        "chunkSafeTensorsWithBoundarySplit (Header)"
    );
    chunks.insert(chunks.end(), headerChunks.begin(), headerChunks.end());

    // 2. Tensor Payload Slice (Static numerical weights)
    auto tensorChunks = chunkSlice(
        data + headerSize,
        size - headerSize,
        headerSize,
        ChunkCategory::TensorPayload,
        TENSOR_PROFILE,
        "chunkSafeTensorsWithBoundarySplit (Weights)"
    );
    chunks.insert(chunks.end(), tensorChunks.begin(), tensorChunks.end());

    return chunks;
}

std::vector<StructuralChunk> AsymmetricChunker::chunkParquetWithStructuralSlices(
    const uint8_t* data,
    size_t size,
    const InspectionResult& inspection
)
{
    std::vector<StructuralChunk> chunks;

    // 1. Initial 4-byte 'PAR1' magic header
    std::string magicHeader(reinterpret_cast<const char*>(data), 4);
    chunks.push_back({
        0,
        4,
        Core::calcSHA256(magicHeader),
        ChunkCategory::MetadataHeader,
        "chunkParquetWithStructuralSlices (Header PAR1)"
    });

    // 2. Middle columnar record blocks / row groups
    size_t middleSize = size - 8;
    auto middleChunks = chunkSlice(
        data + 4,
        middleSize,
        4,
        ChunkCategory::DatasetPayload,
        GENERIC_PROFILE,
        "chunkParquetWithStructuralSlices (RowGroup)"
    );
    chunks.insert(chunks.end(), middleChunks.begin(), middleChunks.end());

    // 3. Trailing 4-byte 'PAR1' magic footer
    std::string magicFooter(reinterpret_cast<const char*>(data + size - 4), 4);
    chunks.push_back({
        size - 4,
        4,
        Core::calcSHA256(magicFooter),
        ChunkCategory::MetadataHeader,
        "chunkParquetWithStructuralSlices (Footer PAR1)"
    });

    return chunks;
}

std::vector<StructuralChunk> AsymmetricChunker::chunkMediaAsAtomicBlob(
    const uint8_t* data,
    size_t size,
    const InspectionResult& inspection
)
{
    std::vector<StructuralChunk> chunks;
    if (size <= 16 * 1024 * 1024)
    {
        std::string mediaData(reinterpret_cast<const char*>(data), size);
        chunks.push_back({
            0,
            size,
            Core::calcSHA256(mediaData),
            ChunkCategory::MediaPayload,
            "chunkMediaAsAtomicBlob"
        });
        return chunks;
    }

    return chunkSlice(
        data,
        size,
        0,
        ChunkCategory::MediaPayload,
        TENSOR_PROFILE,
        "chunkMediaAsAtomicBlob (Streamed)"
    );
}

std::vector<StructuralChunk> AsymmetricChunker::chunkGenericWithFastCDC(
    const uint8_t* data,
    size_t size,
    const InspectionResult& inspection
)
{
    return chunkSlice(
        data,
        size,
        0,
        ChunkCategory::GenericData,
        GENERIC_PROFILE,
        "chunkGenericWithFastCDC"
    );
}

std::vector<StructuralChunk> AsymmetricChunker::chunkBuffer(
    const uint8_t* data,
    size_t size,
    const InspectionResult& inspection
)
{
    if (data == nullptr || size == 0)
    {
        return {};
    }

    // Explicit dispatch based on inspected file format
    if (inspection.format == FileFormat::SafeTensors &&
        inspection.payloadOffset > 0 &&
        inspection.payloadOffset < size)
    {
        return chunkSafeTensorsWithBoundarySplit(data, size, inspection);
    }

    if (inspection.format == FileFormat::Parquet && size > 8)
    {
        return chunkParquetWithStructuralSlices(data, size, inspection);
    }

    if (inspection.format == FileFormat::PNG ||
        inspection.format == FileFormat::JPEG ||
        inspection.format == FileFormat::MP4)
    {
        return chunkMediaAsAtomicBlob(data, size, inspection);
    }

    return chunkGenericWithFastCDC(data, size, inspection);
}

std::vector<StructuralChunk> AsymmetricChunker::chunkFile(
    const std::filesystem::path& filePath,
    const InspectionResult& inspection
)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file for chunking: " + filePath.string());
    }

    file.seekg(0, std::ios::end);
    std::streamoff fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize <= 0)
    {
        return {};
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    return chunkBuffer(buffer.data(), buffer.size(), inspection);
}

} // namespace Core
