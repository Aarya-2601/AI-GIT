#include "../inspection/file_inspector.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

static void printHelp()
{
    std::cout << "\n  🦎 AI-GIT FORMAT INSPECTOR (Zero-Copy Header Sniffer)\n";
    std::cout << "  ─────────────────────────────────────────────────────────────\n";
    std::cout << "  USAGE:\n";
    std::cout << "    aigit-inspect <file_path> [--json]\n\n";
    std::cout << "  OPTIONS:\n";
    std::cout << "    --json        Output result in machine-readable JSON format\n";
    std::cout << "    -h, --help    Show this help message\n\n";
    std::cout << "  SUPPORTED FORMATS & DEDICATED INSPECTION METHODS:\n";
    std::cout << "    • SafeTensors  ➔ inspectSafeTensors (Header length & tensor offset)\n";
    std::cout << "    • GGUF         ➔ inspectGGUF (Quantized model architecture & version)\n";
    std::cout << "    • Parquet      ➔ inspectParquet (PAR1 header & footer validation)\n";
    std::cout << "    • Media        ➔ inspectMedia (PNG, JPEG, MP4 video streams)\n";
    std::cout << "    • Documents    ➔ inspectPDF (PDF document version)\n";
    std::cout << "    • Text / Code  ➔ inspectTextOrBinary (UTF-8 source code & binary)\n\n";
}

static std::string formatBytes(uintmax_t bytes)
{
    const char* suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    int s = 0;
    double count = static_cast<double>(bytes);
    while (count >= 1024.0 && s < 4)
    {
        s++;
        count /= 1024.0;
    }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << count << " " << suffixes[s];
    return ss.str();
}

static std::string escapeJson(const std::string& str)
{
    std::string out;
    for (char c : str)
    {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printHelp();
        return 1;
    }

    std::string targetFile;
    bool jsonOutput = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--json")
        {
            jsonOutput = true;
        }
        else if (arg == "-h" || arg == "--help")
        {
            printHelp();
            return 0;
        }
        else if (targetFile.empty())
        {
            targetFile = arg;
        }
    }

    if (targetFile.empty())
    {
        std::cerr << "Error: No file specified.\n";
        return 1;
    }

    Core::InspectionResult res = Core::FileInspector::inspect(targetFile);

    if (jsonOutput)
    {
        std::cout << "{\n";
        std::cout << "  \"file\": \"" << escapeJson(targetFile) << "\",\n";
        std::cout << "  \"size\": " << res.fileSize << ",\n";
        std::cout << "  \"format\": \"" << res.formatName << "\",\n";
        std::cout << "  \"category\": \"" << Core::FileInspector::categoryToString(res.category) << "\",\n";
        std::cout << "  \"mime_type\": \"" << res.mimeType << "\",\n";
        std::cout << "  \"header_length\": " << res.headerLength << ",\n";
        std::cout << "  \"payload_offset\": " << res.payloadOffset << ",\n";
        std::cout << "  \"detection_method\": \"" << res.detectionMethod << "\",\n";
        std::cout << "  \"details\": \"" << escapeJson(res.details) << "\",\n";
        std::cout << "  \"status\": \"" << (res.format != Core::FileFormat::Unknown ? "ok" : "unknown") << "\"\n";
        std::cout << "}\n";
        return (res.format != Core::FileFormat::Unknown) ? 0 : 2;
    }

    std::cout << "\n";
    std::cout << "  🦎 AI-GIT FILE INSPECTOR\n";
    std::cout << "  ───────────────────────────────────────────────────────────────────\n";
    std::cout << "  │ File Path      : " << targetFile << "\n";
    std::cout << "  │ File Size      : " << formatBytes(res.fileSize) << " (" << res.fileSize << " bytes)\n";
    std::cout << "  │ Detected Format: " << res.formatName << "\n";
    std::cout << "  │ Category       : " << Core::FileInspector::categoryToString(res.category) << "\n";
    std::cout << "  │ MIME Type      : " << res.mimeType << "\n";
    std::cout << "  │ Method Used    : " << res.detectionMethod << "()\n";
    if (res.headerLength > 0 || res.payloadOffset > 0)
    {
        std::cout << "  │ Header Length  : " << res.headerLength << " bytes\n";
        std::cout << "  │ Payload Offset : byte " << res.payloadOffset << " (0x" << std::hex << std::uppercase << res.payloadOffset << std::dec << ")\n";
    }
    std::cout << "  │ Details        : " << res.details << "\n";
    std::cout << "  ───────────────────────────────────────────────────────────────────\n";

    if (res.format == Core::FileFormat::SafeTensors)
    {
        std::cout << "  ✔ Fast Zero-Copy Header Match: Boundary chunking can isolate\n";
        std::cout << "    the volatile JSON metadata from the static " << formatBytes(res.fileSize - res.payloadOffset) << " tensor payload!\n\n";
    }
    else if (res.format == Core::FileFormat::PNG || res.format == Core::FileFormat::JPEG || res.format == Core::FileFormat::MP4)
    {
        std::cout << "  ✔ Pre-compressed Media Detected: Polymorphic codec can bypass\n";
        std::cout << "    CPU compression to eliminate negative compression bloat.\n\n";
    }
    else
    {
        std::cout << "  ✔ Header inspected successfully.\n\n";
    }

    return (res.format != Core::FileFormat::Unknown) ? 0 : 2;
}
