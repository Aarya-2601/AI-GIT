#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace Core
{

enum class FileFormat
{
    SafeTensors,
    GGUF,
    Parquet,
    PNG,
    JPEG,
    MP4,
    PDF,
    Text,
    Binary,
    Unknown
};

enum class FileCategory
{
    ModelWeights,
    Dataset,
    Media,
    Document,
    Text,
    Binary,
    Unknown
};

struct InspectionResult
{
    FileFormat format = FileFormat::Unknown;
    FileCategory category = FileCategory::Unknown;
    std::string formatName = "Unknown";
    std::string mimeType = "application/octet-stream";
    uintmax_t fileSize = 0;
    uint64_t headerLength = 0;     // Size of header/metadata block in bytes (e.g. SafeTensors JSON)
    uint64_t payloadOffset = 0;    // Exact byte offset where tensor/data payload begins
    std::string details;           // Extra parsed info (e.g., "SafeTensors Header: 173 bytes")
    std::string detectionMethod;   // Explicit name of method that handled this file type
};

class FileInspector
{
public:
    // Performs zero-copy/minimal-read inspection (reads up to 64 bytes, plus 4-byte footer for Parquet)
    static InspectionResult inspect(const std::filesystem::path& filePath);

    // Inspects an in-memory buffer (first 16-64 bytes) with optional total file size
    static InspectionResult inspectBuffer(
        const uint8_t* buffer,
        size_t size,
        uintmax_t totalFileSize = 0,
        const std::filesystem::path* filePath = nullptr
    );

    // Explicit format-specific inspection methods
    static bool inspectSafeTensors(const uint8_t* buffer, size_t size, uintmax_t totalFileSize, InspectionResult& out);
    static bool inspectGGUF(const uint8_t* buffer, size_t size, uintmax_t totalFileSize, InspectionResult& out);
    static bool inspectParquet(const uint8_t* buffer, size_t size, uintmax_t totalFileSize, const std::filesystem::path* filePath, InspectionResult& out);
    static bool inspectMedia(const uint8_t* buffer, size_t size, uintmax_t totalFileSize, InspectionResult& out);
    static bool inspectPDF(const uint8_t* buffer, size_t size, uintmax_t totalFileSize, InspectionResult& out);
    static bool inspectTextOrBinary(const uint8_t* buffer, size_t size, uintmax_t totalFileSize, InspectionResult& out);

    static const char* formatToString(FileFormat fmt);
    static const char* categoryToString(FileCategory cat);
};

} // namespace Core
