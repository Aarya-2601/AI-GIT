#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include "../inspection/file_inspector.hpp"
#include "../chunking/asymmetric_chunker.hpp"

namespace Core
{

enum class CodecType : uint8_t
{
    Bypass = 0,            // Pre-compressed media (PNG, JPEG, MP4) - 0% CPU wasted, no expansion
    ZlibStandard = 1,      // Volatile metadata, JSON headers, Parquet headers, source code
    ByteShuffleFP32 = 2,   // 4-byte float byte-shuffle + zlib (SafeTensors/GGUF weights)
    ByteShuffleFP16 = 3,   // 2-byte float/bfloat16 byte-shuffle + zlib
    GenericAdaptive = 4    // General binary with incompressibility detector
};

struct CodecResult
{
    CodecType codecUsed = CodecType::Bypass;
    std::string codecMethod;      // Explicit method name (e.g. "compressTensorWithByteShuffle")
    size_t rawSize = 0;
    size_t compressedSize = 0;
    double compressionRatio = 1.0; // rawSize / compressedSize
    double spaceSavingPercent = 0.0;
    std::string rawSha256;        // Bit-exact CAS key computed on uncompressed bytes
    std::vector<uint8_t> payload; // Serialized container (80-byte header + payload)
};

class PolymorphicCodec
{
public:
    // Explicit format-specific compression methods
    static CodecResult compressMediaWithBypass(
        const uint8_t* rawData,
        size_t size
    );

    static CodecResult compressTensorWithByteShuffle(
        const uint8_t* rawData,
        size_t size,
        size_t elementSize = 4
    );

    static CodecResult compressMetadataWithZlib(
        const uint8_t* rawData,
        size_t size
    );

    static CodecResult compressGenericBinary(
        const uint8_t* rawData,
        size_t size
    );

    // Dispatches compression based on chunk category and inspected file format
    static CodecResult compressChunk(
        const uint8_t* rawData,
        size_t size,
        ChunkCategory category,
        FileFormat format
    );

    // Explicit format-specific decompression methods
    static std::vector<uint8_t> decompressMediaWithBypass(
        const uint8_t* payloadData,
        size_t payloadSize
    );

    static std::vector<uint8_t> decompressTensorWithByteUnshuffle(
        const uint8_t* compressedData,
        size_t compressedSize,
        size_t rawSize,
        size_t elementSize
    );

    static std::vector<uint8_t> decompressMetadataWithZlib(
        const uint8_t* compressedData,
        size_t compressedSize
    );

    static std::vector<uint8_t> decompressGenericBinary(
        const uint8_t* compressedData,
        size_t compressedSize
    );

    // Universal container decompression & CAS integrity validation
    static std::vector<uint8_t> decompressContainer(
        const uint8_t* containerData,
        size_t containerSize,
        std::string* outRawSha256 = nullptr,
        std::string* outMethod = nullptr
    );

    // Low-level byte-shuffle algorithms (SIMD-friendly transposed byte planes)
    static std::vector<uint8_t> shuffleBytes(const uint8_t* data, size_t size, size_t elementSize);
    static std::vector<uint8_t> unshuffleBytes(const uint8_t* data, size_t size, size_t elementSize);

    static const char* codecTypeToString(CodecType type);
};

} // namespace Core
