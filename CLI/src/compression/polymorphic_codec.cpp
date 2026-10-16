#include "polymorphic_codec.hpp"
#include "compression.hpp"
#include "../hashing/hashing.hpp"

#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace Core
{

#pragma pack(push, 1)
struct ContainerHeader
{
    char magic[4];          // "AGC1" (AI-Git Codec v1)
    uint8_t codecType;      // CodecType enum value
    uint8_t elementSize;    // e.g. 4 for FP32, 2 for FP16, 0 for others
    uint16_t flags;         // Reserved flags
    uint64_t rawSize;       // Uncompressed data size in bytes
    char rawSha256[64];     // Bit-exact uncompressed SHA-256 CAS address
};
#pragma pack(pop)

static_assert(sizeof(ContainerHeader) == 80, "ContainerHeader must be exactly 80 bytes");

const char* PolymorphicCodec::codecTypeToString(CodecType type)
{
    switch (type)
    {
        case CodecType::Bypass:          return "Bypass (Zero-CPU)";
        case CodecType::ZlibStandard:    return "ZlibStandard";
        case CodecType::ByteShuffleFP32: return "ByteShuffle_FP32+Zlib";
        case CodecType::ByteShuffleFP16: return "ByteShuffle_FP16+Zlib";
        case CodecType::GenericAdaptive: return "GenericAdaptive";
        default:                         return "Unknown";
    }
}

// ----------------------------------------------------------------------------
// Low-Level Byte Shuffling / Unshuffling (Transposition)
// ----------------------------------------------------------------------------

std::vector<uint8_t> PolymorphicCodec::shuffleBytes(const uint8_t* data, size_t size, size_t elementSize)
{
    if (data == nullptr || size == 0)
    {
        return {};
    }

    if (elementSize <= 1 || size <= elementSize)
    {
        return std::vector<uint8_t>(data, data + size);
    }

    size_t numElements = size / elementSize;
    size_t remainder = size % elementSize;

    std::vector<uint8_t> out(size);

    for (size_t b = 0; b < elementSize; ++b)
    {
        size_t planeOffset = b * numElements;
        for (size_t i = 0; i < numElements; ++i)
        {
            out[planeOffset + i] = data[i * elementSize + b];
        }
    }

    if (remainder > 0)
    {
        size_t tailOffset = numElements * elementSize;
        std::memcpy(out.data() + tailOffset, data + tailOffset, remainder);
    }

    return out;
}

std::vector<uint8_t> PolymorphicCodec::unshuffleBytes(const uint8_t* data, size_t size, size_t elementSize)
{
    if (data == nullptr || size == 0)
    {
        return {};
    }

    if (elementSize <= 1 || size <= elementSize)
    {
        return std::vector<uint8_t>(data, data + size);
    }

    size_t numElements = size / elementSize;
    size_t remainder = size % elementSize;

    std::vector<uint8_t> out(size);

    for (size_t b = 0; b < elementSize; ++b)
    {
        size_t planeOffset = b * numElements;
        for (size_t i = 0; i < numElements; ++i)
        {
            out[i * elementSize + b] = data[planeOffset + i];
        }
    }

    if (remainder > 0)
    {
        size_t tailOffset = numElements * elementSize;
        std::memcpy(out.data() + tailOffset, data + tailOffset, remainder);
    }

    return out;
}

// ----------------------------------------------------------------------------
// Explicit Format-Specific Compression Methods
// ----------------------------------------------------------------------------

CodecResult PolymorphicCodec::compressMediaWithBypass(
    const uint8_t* rawData,
    size_t size
)
{
    CodecResult result;
    result.codecUsed = CodecType::Bypass;
    result.codecMethod = "compressMediaWithBypass";
    result.rawSize = size;

    std::string rawDataStr(reinterpret_cast<const char*>(rawData), size);
    result.rawSha256 = Core::calcSHA256(rawDataStr);

    ContainerHeader header;
    std::memcpy(header.magic, "AGC1", 4);
    header.codecType = static_cast<uint8_t>(CodecType::Bypass);
    header.elementSize = 0;
    header.flags = 0;
    header.rawSize = static_cast<uint64_t>(size);
    std::memset(header.rawSha256, 0, sizeof(header.rawSha256));
    std::memcpy(header.rawSha256, result.rawSha256.c_str(), std::min<size_t>(64, result.rawSha256.size()));

    result.payload.resize(sizeof(ContainerHeader) + size);
    std::memcpy(result.payload.data(), &header, sizeof(ContainerHeader));
    if (size > 0 && rawData != nullptr)
    {
        std::memcpy(result.payload.data() + sizeof(ContainerHeader), rawData, size);
    }

    result.compressedSize = result.payload.size();
    result.compressionRatio = (size > 0) ? (static_cast<double>(size) / result.compressedSize) : 1.0;
    result.spaceSavingPercent = 0.0; // Media bypass intentionally does not compress

    return result;
}

CodecResult PolymorphicCodec::compressTensorWithByteShuffle(
    const uint8_t* rawData,
    size_t size,
    size_t elementSize
)
{
    CodecResult result;
    result.codecUsed = (elementSize == 2) ? CodecType::ByteShuffleFP16 : CodecType::ByteShuffleFP32;
    result.codecMethod = (elementSize == 2) ? "compressTensorWithByteShuffle (FP16/BF16)" : "compressTensorWithByteShuffle (FP32)";
    result.rawSize = size;

    std::string rawDataStr(reinterpret_cast<const char*>(rawData), size);
    result.rawSha256 = Core::calcSHA256(rawDataStr);

    if (size == 0)
    {
        return compressMediaWithBypass(rawData, 0);
    }

    // Step 1: Shuffle byte planes (groups low-entropy exponent bytes together)
    std::vector<uint8_t> shuffled = shuffleBytes(rawData, size, elementSize);

    // Step 2: Compress transposed stream with zlib
    std::string shuffledStr(reinterpret_cast<const char*>(shuffled.data()), shuffled.size());
    std::string compressedStream = Core::compressString(shuffledStr);

    // Fallback check: If compression expanded the data, bypass
    if (compressedStream.size() >= size)
    {
        auto bypassRes = compressMediaWithBypass(rawData, size);
        bypassRes.codecMethod = "compressTensorWithByteShuffle (Bypass Fallback)";
        return bypassRes;
    }

    // Step 3: Package into container with AGC1 header
    ContainerHeader header;
    std::memcpy(header.magic, "AGC1", 4);
    header.codecType = static_cast<uint8_t>(result.codecUsed);
    header.elementSize = static_cast<uint8_t>(elementSize);
    header.flags = 0;
    header.rawSize = static_cast<uint64_t>(size);
    std::memset(header.rawSha256, 0, sizeof(header.rawSha256));
    std::memcpy(header.rawSha256, result.rawSha256.c_str(), std::min<size_t>(64, result.rawSha256.size()));

    result.payload.resize(sizeof(ContainerHeader) + compressedStream.size());
    std::memcpy(result.payload.data(), &header, sizeof(ContainerHeader));
    std::memcpy(result.payload.data() + sizeof(ContainerHeader), compressedStream.data(), compressedStream.size());

    result.compressedSize = result.payload.size();
    result.compressionRatio = static_cast<double>(size) / result.compressedSize;
    result.spaceSavingPercent = (1.0 - (static_cast<double>(result.compressedSize) / size)) * 100.0;

    return result;
}

CodecResult PolymorphicCodec::compressMetadataWithZlib(
    const uint8_t* rawData,
    size_t size
)
{
    CodecResult result;
    result.codecUsed = CodecType::ZlibStandard;
    result.codecMethod = "compressMetadataWithZlib";
    result.rawSize = size;

    std::string rawDataStr(reinterpret_cast<const char*>(rawData), size);
    result.rawSha256 = Core::calcSHA256(rawDataStr);

    if (size == 0)
    {
        return compressMediaWithBypass(rawData, 0);
    }

    std::string compressedStream = Core::compressString(rawDataStr);

    if (compressedStream.size() >= size)
    {
        auto bypassRes = compressMediaWithBypass(rawData, size);
        bypassRes.codecMethod = "compressMetadataWithZlib (Bypass Fallback)";
        return bypassRes;
    }

    ContainerHeader header;
    std::memcpy(header.magic, "AGC1", 4);
    header.codecType = static_cast<uint8_t>(CodecType::ZlibStandard);
    header.elementSize = 0;
    header.flags = 0;
    header.rawSize = static_cast<uint64_t>(size);
    std::memset(header.rawSha256, 0, sizeof(header.rawSha256));
    std::memcpy(header.rawSha256, result.rawSha256.c_str(), std::min<size_t>(64, result.rawSha256.size()));

    result.payload.resize(sizeof(ContainerHeader) + compressedStream.size());
    std::memcpy(result.payload.data(), &header, sizeof(ContainerHeader));
    std::memcpy(result.payload.data() + sizeof(ContainerHeader), compressedStream.data(), compressedStream.size());

    result.compressedSize = result.payload.size();
    result.compressionRatio = static_cast<double>(size) / result.compressedSize;
    result.spaceSavingPercent = (1.0 - (static_cast<double>(result.compressedSize) / size)) * 100.0;

    return result;
}

CodecResult PolymorphicCodec::compressGenericBinary(
    const uint8_t* rawData,
    size_t size
)
{
    CodecResult result;
    result.codecUsed = CodecType::GenericAdaptive;
    result.codecMethod = "compressGenericBinary";
    result.rawSize = size;

    std::string rawDataStr(reinterpret_cast<const char*>(rawData), size);
    result.rawSha256 = Core::calcSHA256(rawDataStr);

    if (size <= 16)
    {
        auto bypassRes = compressMediaWithBypass(rawData, size);
        bypassRes.codecMethod = "compressGenericBinary (Small Payload Bypass)";
        return bypassRes;
    }

    std::string compressedStream = Core::compressString(rawDataStr);

    if (compressedStream.size() >= size)
    {
        auto bypassRes = compressMediaWithBypass(rawData, size);
        bypassRes.codecMethod = "compressGenericBinary (Incompressible Bypass)";
        return bypassRes;
    }

    ContainerHeader header;
    std::memcpy(header.magic, "AGC1", 4);
    header.codecType = static_cast<uint8_t>(CodecType::GenericAdaptive);
    header.elementSize = 0;
    header.flags = 0;
    header.rawSize = static_cast<uint64_t>(size);
    std::memset(header.rawSha256, 0, sizeof(header.rawSha256));
    std::memcpy(header.rawSha256, result.rawSha256.c_str(), std::min<size_t>(64, result.rawSha256.size()));

    result.payload.resize(sizeof(ContainerHeader) + compressedStream.size());
    std::memcpy(result.payload.data(), &header, sizeof(ContainerHeader));
    std::memcpy(result.payload.data() + sizeof(ContainerHeader), compressedStream.data(), compressedStream.size());

    result.compressedSize = result.payload.size();
    result.compressionRatio = static_cast<double>(size) / result.compressedSize;
    result.spaceSavingPercent = (1.0 - (static_cast<double>(result.compressedSize) / size)) * 100.0;

    return result;
}

CodecResult PolymorphicCodec::compressChunk(
    const uint8_t* rawData,
    size_t size,
    ChunkCategory category,
    FileFormat format
)
{
    // 1. Metadata / JSON headers always get high-efficiency text compression
    if (category == ChunkCategory::MetadataHeader ||
        format == FileFormat::Text ||
        format == FileFormat::PDF)
    {
        return compressMetadataWithZlib(rawData, size);
    }

    // 2. Pre-compressed media always bypasses compression (0% CPU wasted, no expansion)
    if (category == ChunkCategory::MediaPayload ||
        format == FileFormat::PNG ||
        format == FileFormat::JPEG ||
        format == FileFormat::MP4)
    {
        return compressMediaWithBypass(rawData, size);
    }

    // 3. Tensor weights get transposed byte-shuffling (FP32 planes)
    if (category == ChunkCategory::TensorPayload ||
        format == FileFormat::SafeTensors ||
        format == FileFormat::GGUF)
    {
        return compressTensorWithByteShuffle(rawData, size, 4);
    }

    return compressGenericBinary(rawData, size);
}

// ----------------------------------------------------------------------------
// Decompression & CAS Integrity Verification
// ----------------------------------------------------------------------------

std::vector<uint8_t> PolymorphicCodec::decompressMediaWithBypass(
    const uint8_t* payloadData,
    size_t payloadSize
)
{
    if (payloadData == nullptr || payloadSize == 0)
    {
        return {};
    }
    return std::vector<uint8_t>(payloadData, payloadData + payloadSize);
}

std::vector<uint8_t> PolymorphicCodec::decompressTensorWithByteUnshuffle(
    const uint8_t* compressedData,
    size_t compressedSize,
    size_t rawSize,
    size_t elementSize
)
{
    if (compressedData == nullptr || compressedSize == 0 || rawSize == 0)
    {
        return {};
    }

    std::string compStr(reinterpret_cast<const char*>(compressedData), compressedSize);
    std::string decompStr = Core::decompressData(compStr);

    if (decompStr.size() != rawSize)
    {
        throw std::runtime_error("Decompressed size mismatch in tensor stream");
    }

    return unshuffleBytes(reinterpret_cast<const uint8_t*>(decompStr.data()), rawSize, elementSize);
}

std::vector<uint8_t> PolymorphicCodec::decompressMetadataWithZlib(
    const uint8_t* compressedData,
    size_t compressedSize
)
{
    if (compressedData == nullptr || compressedSize == 0)
    {
        return {};
    }

    std::string compStr(reinterpret_cast<const char*>(compressedData), compressedSize);
    std::string decompStr = Core::decompressData(compStr);

    return std::vector<uint8_t>(decompStr.begin(), decompStr.end());
}

std::vector<uint8_t> PolymorphicCodec::decompressGenericBinary(
    const uint8_t* compressedData,
    size_t compressedSize
)
{
    return decompressMetadataWithZlib(compressedData, compressedSize);
}

std::vector<uint8_t> PolymorphicCodec::decompressContainer(
    const uint8_t* containerData,
    size_t containerSize,
    std::string* outRawSha256,
    std::string* outMethod
)
{
    if (containerData == nullptr || containerSize < sizeof(ContainerHeader))
    {
        throw std::runtime_error("Invalid container: buffer is smaller than header");
    }

    ContainerHeader header;
    std::memcpy(&header, containerData, sizeof(ContainerHeader));

    if (std::memcmp(header.magic, "AGC1", 4) != 0)
    {
        throw std::runtime_error("Invalid container magic: expected AGC1");
    }

    std::string expectedSha256(header.rawSha256, strnlen(header.rawSha256, 64));
    if (outRawSha256 != nullptr)
    {
        *outRawSha256 = expectedSha256;
    }

    const uint8_t* payload = containerData + sizeof(ContainerHeader);
    size_t payloadSize = containerSize - sizeof(ContainerHeader);

    std::vector<uint8_t> restored;
    std::string methodUsed;

    switch (static_cast<CodecType>(header.codecType))
    {
        case CodecType::Bypass:
            methodUsed = "decompressMediaWithBypass";
            restored = decompressMediaWithBypass(payload, payloadSize);
            break;

        case CodecType::ByteShuffleFP32:
            methodUsed = "decompressTensorWithByteUnshuffle (FP32)";
            restored = decompressTensorWithByteUnshuffle(payload, payloadSize, static_cast<size_t>(header.rawSize), 4);
            break;

        case CodecType::ByteShuffleFP16:
            methodUsed = "decompressTensorWithByteUnshuffle (FP16)";
            restored = decompressTensorWithByteUnshuffle(payload, payloadSize, static_cast<size_t>(header.rawSize), 2);
            break;

        case CodecType::ZlibStandard:
            methodUsed = "decompressMetadataWithZlib";
            restored = decompressMetadataWithZlib(payload, payloadSize);
            break;

        case CodecType::GenericAdaptive:
            methodUsed = "decompressGenericBinary";
            restored = decompressGenericBinary(payload, payloadSize);
            break;

        default:
            throw std::runtime_error("Unknown codec type in container header");
    }

    if (outMethod != nullptr)
    {
        *outMethod = methodUsed;
    }

    // Verify bit-exact CAS integrity:
    std::string restoredStr(reinterpret_cast<const char*>(restored.data()), restored.size());
    std::string actualSha256 = Core::calcSHA256(restoredStr);

    if (actualSha256 != expectedSha256)
    {
        std::ostringstream err;
        err << "CAS CHECKSUM CORRUPTION! Expected " << expectedSha256 << " but got " << actualSha256;
        throw std::runtime_error(err.str());
    }

    return restored;
}

} // namespace Core
