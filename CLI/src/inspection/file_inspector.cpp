#include "file_inspector.hpp"

#include <fstream>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace Core
{

const char* FileInspector::formatToString(FileFormat fmt)
{
    switch (fmt)
    {
        case FileFormat::SafeTensors: return "SafeTensors";
        case FileFormat::GGUF:        return "GGUF";
        case FileFormat::Parquet:     return "Parquet";
        case FileFormat::PNG:         return "PNG";
        case FileFormat::JPEG:        return "JPEG";
        case FileFormat::MP4:         return "MP4";
        case FileFormat::PDF:         return "PDF";
        case FileFormat::Text:        return "Text";
        case FileFormat::Binary:      return "Binary";
        default:                      return "Unknown";
    }
}

const char* FileInspector::categoryToString(FileCategory cat)
{
    switch (cat)
    {
        case FileCategory::ModelWeights: return "ModelWeights";
        case FileCategory::Dataset:      return "Dataset";
        case FileCategory::Media:        return "Media";
        case FileCategory::Document:     return "Document";
        case FileCategory::Text:         return "Text";
        case FileCategory::Binary:       return "Binary";
        default:                         return "Unknown";
    }
}

static bool isPrintableText(const uint8_t* buffer, size_t size)
{
    if (size == 0) return true;
    size_t printableCount = 0;
    for (size_t i = 0; i < size; ++i)
    {
        uint8_t c = buffer[i];
        if (c == 0)
        {
            return false;
        }
        if ((c >= 0x20 && c <= 0x7E) || c == '\t' || c == '\r' || c == '\n')
        {
            printableCount++;
        }
        else if (c >= 0x80)
        {
            printableCount++;
        }
    }
    return (printableCount * 100 / size) >= 90;
}

// ----------------------------------------------------------------------------
// Explicit File-Type Inspection Methods
// ----------------------------------------------------------------------------

bool FileInspector::inspectSafeTensors(const uint8_t* buffer, size_t size, uintmax_t totalFileSize, InspectionResult& out)
{
    if (size < 10) return false;

    uint64_t headerLen = static_cast<uint64_t>(buffer[0])
                       | (static_cast<uint64_t>(buffer[1]) << 8)
                       | (static_cast<uint64_t>(buffer[2]) << 16)
                       | (static_cast<uint64_t>(buffer[3]) << 24)
                       | (static_cast<uint64_t>(buffer[4]) << 32)
                       | (static_cast<uint64_t>(buffer[5]) << 40)
                       | (static_cast<uint64_t>(buffer[6]) << 48)
                       | (static_cast<uint64_t>(buffer[7]) << 56);

    if (headerLen > 0 && headerLen < 100 * 1024 * 1024 && (buffer[8] == '{' || buffer[8] == ' '))
    {
        size_t firstNonWhitespace = 8;
        while (firstNonWhitespace < size && buffer[firstNonWhitespace] == ' ')
        {
            firstNonWhitespace++;
        }
        if (firstNonWhitespace < size && buffer[firstNonWhitespace] == '{')
        {
            out.format = FileFormat::SafeTensors;
            out.category = FileCategory::ModelWeights;
            out.formatName = "SafeTensors";
            out.mimeType = "application/x-safetensors";
            out.headerLength = headerLen;
            out.payloadOffset = 8 + headerLen;
            out.detectionMethod = "inspectSafeTensors";

            std::ostringstream ss;
            ss << "SafeTensors Header: " << headerLen << " bytes, Payload starts at offset " << out.payloadOffset;
            out.details = ss.str();
            return true;
        }
    }
    return false;
}

bool FileInspector::inspectGGUF(const uint8_t* buffer, size_t size, uintmax_t totalFileSize, InspectionResult& out)
{
    if (size < 8) return false;

    if (buffer[0] == 'G' && buffer[1] == 'G' && buffer[2] == 'U' && buffer[3] == 'F')
    {
        uint32_t version = static_cast<uint32_t>(buffer[4])
                         | (static_cast<uint32_t>(buffer[5]) << 8)
                         | (static_cast<uint32_t>(buffer[6]) << 16)
                         | (static_cast<uint32_t>(buffer[7]) << 24);

        out.format = FileFormat::GGUF;
        out.category = FileCategory::ModelWeights;
        out.formatName = "GGUF";
        out.mimeType = "application/x-gguf";
        out.headerLength = 8;
        out.payloadOffset = 8;
        out.detectionMethod = "inspectGGUF";

        std::ostringstream ss;
        ss << "GGUF Model Container (Format Version " << version << ")";
        out.details = ss.str();
        return true;
    }
    return false;
}

bool FileInspector::inspectParquet(const uint8_t* buffer, size_t size, uintmax_t totalFileSize, const std::filesystem::path* filePath, InspectionResult& out)
{
    if (size < 4) return false;

    if (buffer[0] == 'P' && buffer[1] == 'A' && buffer[2] == 'R' && buffer[3] == '1')
    {
        out.format = FileFormat::Parquet;
        out.category = FileCategory::Dataset;
        out.formatName = "Parquet";
        out.mimeType = "application/vnd.apache.parquet";
        out.headerLength = 4;
        out.payloadOffset = 4;
        out.detectionMethod = "inspectParquet";

        bool footerValid = false;
        if (filePath != nullptr && totalFileSize >= 8)
        {
            std::ifstream file(*filePath, std::ios::binary);
            if (file.is_open())
            {
                file.seekg(static_cast<std::streamoff>(totalFileSize - 4));
                char footerMagic[4] = {0};
                file.read(footerMagic, 4);
                if (file.gcount() == 4 && std::memcmp(footerMagic, "PAR1", 4) == 0)
                {
                    footerValid = true;
                }
            }
        }
        else if (totalFileSize >= 8 && size >= totalFileSize)
        {
            if (std::memcmp(buffer + totalFileSize - 4, "PAR1", 4) == 0)
            {
                footerValid = true;
            }
        }

        if (footerValid)
        {
            out.details = "Apache Parquet Columnar Dataset (Valid PAR1 Header & Footer)";
        }
        else
        {
            out.details = "Apache Parquet Columnar Dataset (PAR1 Header Verified)";
        }
        return true;
    }
    return false;
}

bool FileInspector::inspectMedia(const uint8_t* buffer, size_t size, uintmax_t totalFileSize, InspectionResult& out)
{
    // PNG Check: 89 50 4E 47 0D 0A 1A 0A
    if (size >= 8 &&
        buffer[0] == 0x89 && buffer[1] == 0x50 && buffer[2] == 0x4E && buffer[3] == 0x47 &&
        buffer[4] == 0x0D && buffer[5] == 0x0A && buffer[6] == 0x1A && buffer[7] == 0x0A)
    {
        out.format = FileFormat::PNG;
        out.category = FileCategory::Media;
        out.formatName = "PNG";
        out.mimeType = "image/png";
        out.headerLength = 8;
        out.payloadOffset = 0;
        out.details = "Portable Network Graphics Image";
        out.detectionMethod = "inspectMedia";
        return true;
    }

    // JPEG Check: FF D8 FF
    if (size >= 3 && buffer[0] == 0xFF && buffer[1] == 0xD8 && buffer[2] == 0xFF)
    {
        out.format = FileFormat::JPEG;
        out.category = FileCategory::Media;
        out.formatName = "JPEG";
        out.mimeType = "image/jpeg";
        out.headerLength = 3;
        out.payloadOffset = 0;
        out.details = "JPEG Image";
        out.detectionMethod = "inspectMedia";
        return true;
    }

    // MP4 Check: offset 4 contains 'ftyp'
    if (size >= 12 && buffer[4] == 'f' && buffer[5] == 't' && buffer[6] == 'y' && buffer[7] == 'p')
    {
        out.format = FileFormat::MP4;
        out.category = FileCategory::Media;
        out.formatName = "MP4";
        out.mimeType = "video/mp4";
        out.headerLength = 8;
        out.payloadOffset = 0;
        char brand[5] = {0};
        std::memcpy(brand, buffer + 8, 4);
        out.details = std::string("ISO Base Media / MP4 Video (Brand: ") + brand + ")";
        out.detectionMethod = "inspectMedia";
        return true;
    }

    return false;
}

bool FileInspector::inspectPDF(const uint8_t* buffer, size_t size, uintmax_t totalFileSize, InspectionResult& out)
{
    // PDF Check: %PDF-
    if (size >= 5 && buffer[0] == '%' && buffer[1] == 'P' && buffer[2] == 'D' && buffer[3] == 'F' && buffer[4] == '-')
    {
        out.format = FileFormat::PDF;
        out.category = FileCategory::Document;
        out.formatName = "PDF";
        out.mimeType = "application/pdf";
        out.headerLength = 5;
        out.payloadOffset = 0;
        out.detectionMethod = "inspectPDF";

        std::string version = "1.x";
        if (size >= 8)
        {
            version = std::string(reinterpret_cast<const char*>(buffer + 5), 3);
        }
        out.details = "Adobe Portable Document Format (PDF " + version + ")";
        return true;
    }
    return false;
}

bool FileInspector::inspectTextOrBinary(const uint8_t* buffer, size_t size, uintmax_t totalFileSize, InspectionResult& out)
{
    if (isPrintableText(buffer, size))
    {
        out.format = FileFormat::Text;
        out.category = FileCategory::Text;
        out.formatName = "Text";
        out.mimeType = "text/plain";
        out.headerLength = 0;
        out.payloadOffset = 0;
        out.details = "Plaintext / Source Code";
        out.detectionMethod = "inspectTextOrBinary";
        return true;
    }

    out.format = FileFormat::Binary;
    out.category = FileCategory::Binary;
    out.formatName = "Binary";
    out.mimeType = "application/octet-stream";
    out.headerLength = 0;
    out.payloadOffset = 0;
    out.details = "Generic Opaque Binary Stream";
    out.detectionMethod = "inspectTextOrBinary";
    return true;
}

InspectionResult FileInspector::inspectBuffer(
    const uint8_t* buffer,
    size_t size,
    uintmax_t totalFileSize,
    const std::filesystem::path* filePath
)
{
    InspectionResult result;
    result.fileSize = totalFileSize;

    if (buffer == nullptr || size == 0)
    {
        result.format = FileFormat::Unknown;
        result.category = FileCategory::Unknown;
        result.formatName = "Empty";
        result.mimeType = "application/x-empty";
        result.details = "Empty file or buffer";
        result.detectionMethod = "none";
        return result;
    }

    // Try format-specific methods in order
    if (inspectSafeTensors(buffer, size, totalFileSize, result)) return result;
    if (inspectGGUF(buffer, size, totalFileSize, result)) return result;
    if (inspectParquet(buffer, size, totalFileSize, filePath, result)) return result;
    if (inspectMedia(buffer, size, totalFileSize, result)) return result;
    if (inspectPDF(buffer, size, totalFileSize, result)) return result;
    inspectTextOrBinary(buffer, size, totalFileSize, result);
    return result;
}

InspectionResult FileInspector::inspect(const std::filesystem::path& filePath)
{
    InspectionResult result;

    if (!std::filesystem::exists(filePath))
    {
        result.format = FileFormat::Unknown;
        result.category = FileCategory::Unknown;
        result.details = "File does not exist: " + filePath.string();
        result.detectionMethod = "none";
        return result;
    }

    std::error_code ec;
    uintmax_t fileSize = std::filesystem::file_size(filePath, ec);
    if (ec)
    {
        result.format = FileFormat::Unknown;
        result.category = FileCategory::Unknown;
        result.details = "Failed to query file size: " + ec.message();
        result.detectionMethod = "none";
        return result;
    }

    result.fileSize = fileSize;

    if (fileSize == 0)
    {
        result.format = FileFormat::Unknown;
        result.category = FileCategory::Unknown;
        result.formatName = "Empty";
        result.mimeType = "application/x-empty";
        result.details = "Empty file (0 bytes)";
        result.detectionMethod = "none";
        return result;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        result.format = FileFormat::Unknown;
        result.category = FileCategory::Unknown;
        result.details = "Failed to open file for inspection";
        result.detectionMethod = "none";
        return result;
    }

    uint8_t buffer[64];
    size_t toRead = static_cast<size_t>(std::min<uintmax_t>(fileSize, sizeof(buffer)));
    file.read(reinterpret_cast<char*>(buffer), toRead);
    std::streamsize bytesRead = file.gcount();
    file.close();

    if (bytesRead <= 0)
    {
        result.format = FileFormat::Unknown;
        result.category = FileCategory::Unknown;
        result.details = "Failed to read header bytes";
        result.detectionMethod = "none";
        return result;
    }

    return inspectBuffer(buffer, static_cast<size_t>(bytesRead), fileSize, &filePath);
}

} // namespace Core
