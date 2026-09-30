#include "../chunking/asymmetric_chunker.hpp"
#include "../inspection/file_inspector.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

static void printHelp()
{
    std::cout << "\n  🦎 AI-GIT ASYMMETRIC CHUNKER (Boundary-Aligned FastCDC)\n";
    std::cout << "  ─────────────────────────────────────────────────────────────\n";
    std::cout << "  USAGE:\n";
    std::cout << "    aigit-chunk <file_path> [--json]\n\n";
    std::cout << "  FEATURES & DEDICATED FILE-TYPE METHODS:\n";
    std::cout << "    • SafeTensors  ➔ chunkSafeTensorsWithBoundarySplit (Header vs Weights)\n";
    std::cout << "    • Parquet      ➔ chunkParquetWithStructuralSlices (PAR1 Header & Footer)\n";
    std::cout << "    • Media        ➔ chunkMediaAsAtomicBlob (Atomic preservation <= 16MB)\n";
    std::cout << "    • Generic      ➔ chunkGenericWithFastCDC (Parameterized dynamic FastCDC)\n\n";
}

static std::string formatBytes(size_t bytes)
{
    const char* suffixes[] = {"B", "KB", "MB", "GB"};
    int s = 0;
    double count = static_cast<double>(bytes);
    while (count >= 1024.0 && s < 3)
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

    if (!std::filesystem::exists(targetFile))
    {
        std::cerr << "Error: File does not exist: " << targetFile << "\n";
        return 1;
    }

    // Step 1: Zero-copy inspection
    Core::InspectionResult inspection = Core::FileInspector::inspect(targetFile);

    // Step 2: Asymmetric chunking
    std::vector<Core::StructuralChunk> chunks = Core::AsymmetricChunker::chunkFile(targetFile, inspection);

    if (jsonOutput)
    {
        std::cout << "{\n";
        std::cout << "  \"file\": \"" << escapeJson(targetFile) << "\",\n";
        std::cout << "  \"size\": " << inspection.fileSize << ",\n";
        std::cout << "  \"format\": \"" << inspection.formatName << "\",\n";
        std::cout << "  \"category\": \"" << Core::FileInspector::categoryToString(inspection.category) << "\",\n";
        std::cout << "  \"payload_offset\": " << inspection.payloadOffset << ",\n";
        std::cout << "  \"chunk_count\": " << chunks.size() << ",\n";
        std::cout << "  \"chunks\": [\n";

        for (size_t i = 0; i < chunks.size(); ++i)
        {
            const auto& ch = chunks[i];
            std::cout << "    {\n";
            std::cout << "      \"index\": " << i << ",\n";
            std::cout << "      \"offset\": " << ch.offset << ",\n";
            std::cout << "      \"length\": " << ch.length << ",\n";
            std::cout << "      \"category\": \"" << Core::AsymmetricChunker::categoryToString(ch.category) << "\",\n";
            std::cout << "      \"method\": \"" << escapeJson(ch.processingMethod) << "\",\n";
            std::cout << "      \"sha256\": \"" << ch.sha256 << "\"\n";
            std::cout << "    }" << (i + 1 < chunks.size() ? "," : "") << "\n";
        }

        std::cout << "  ]\n";
        std::cout << "}\n";
        return 0;
    }

    std::cout << "\n";
    std::cout << "  🦎 AI-GIT ASYMMETRIC CHUNKER\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────────────────────────────────\n";
    std::cout << "  │ Target File    : " << targetFile << "\n";
    std::cout << "  │ File Size      : " << formatBytes(inspection.fileSize) << " (" << inspection.fileSize << " bytes)\n";
    std::cout << "  │ Format Detected: " << inspection.formatName << " (" << Core::FileInspector::categoryToString(inspection.category) << ")\n";
    if (inspection.payloadOffset > 0)
    {
        std::cout << "  │ Structural Snap: Header [0.." << inspection.payloadOffset << ") ➔ Payload [" << inspection.payloadOffset << ".." << inspection.fileSize << ")\n";
    }
    std::cout << "  │ Total Chunks   : " << chunks.size() << "\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────────────────────────────────\n";
    std::cout << "  " << std::left << std::setw(6)  << "CHUNK"
              << std::left << std::setw(16) << "CATEGORY"
              << std::left << std::setw(10) << "OFFSET"
              << std::left << std::setw(10) << "LENGTH"
              << std::left << std::setw(42) << "PROCESSING METHOD"
              << "SHA-256 (PREFIX)\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────────────────────────────────\n";

    for (size_t i = 0; i < chunks.size(); ++i)
    {
        const auto& ch = chunks[i];
        std::cout << "  "
                  << "#" << std::left << std::setw(5) << (i + 1)
                  << std::left << std::setw(16) << Core::AsymmetricChunker::categoryToString(ch.category)
                  << std::left << std::setw(10) << ch.offset
                  << std::left << std::setw(10) << formatBytes(ch.length)
                  << std::left << std::setw(42) << ch.processingMethod
                  << ch.sha256.substr(0, 16) << "...\n";
    }

    std::cout << "  ─────────────────────────────────────────────────────────────────────────────────────────────\n";
    if (inspection.format == Core::FileFormat::SafeTensors)
    {
        std::cout << "  ✔ Structural alignment active: Changes to JSON hyperparameters will only invalidate\n";
        std::cout << "    the MetadataHeader chunk, leaving all TensorPayload chunks 100% deduplicated!\n\n";
    }
    else
    {
        std::cout << "  ✔ Chunks generated successfully with structural boundary snapping.\n\n";
    }

    return 0;
}
