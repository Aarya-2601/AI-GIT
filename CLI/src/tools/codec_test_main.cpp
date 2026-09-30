#include "../compression/polymorphic_codec.hpp"
#include "../inspection/file_inspector.hpp"
#include "../chunking/asymmetric_chunker.hpp"
#include "../hashing/hashing.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

static void printHelp()
{
    std::cout << "\n  🦎 AI-GIT POLYMORPHIC CODEC VERIFICATION TOOL\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────────────────\n";
    std::cout << "  USAGE:\n";
    std::cout << "    aigit-codec inspect <file> [--json]               Inspect polymorphic codec per chunk\n";
    std::cout << "    aigit-codec compress <file> <out_archive>         Compress file into AGC1 container\n";
    std::cout << "    aigit-codec decompress <in_archive> <out_file>    Decompress & verify bit-exact CAS hash\n";
    std::cout << "    aigit-codec bench <file>                          Benchmark plain vs polymorphic codec\n\n";
    std::cout << "  FORMAT-SPECIFIC CODEC METHODS:\n";
    std::cout << "    • PNG / JPEG / MP4 ➔ compressMediaWithBypass (Zero CPU wasted, 0 expansion)\n";
    std::cout << "    • SafeTensors / GGUF ➔ compressTensorWithByteShuffle (Transposed FP32/FP16 planes)\n";
    std::cout << "    • JSON / Text / Code ➔ compressMetadataWithZlib (High ratio entropy compression)\n";
    std::cout << "    • Datasets / Binary ➔ compressGenericBinary (Adaptive incompressibility check)\n\n";
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

    std::string command = argv[1];

    if (command == "-h" || command == "--help")
    {
        printHelp();
        return 0;
    }

    // ------------------------------------------------------------------------
    // COMMAND: inspect
    // ------------------------------------------------------------------------
    if (command == "inspect")
    {
        if (argc < 3)
        {
            std::cerr << "Error: File path required for inspect.\n";
            return 1;
        }

        std::string filePath = argv[2];
        bool jsonOutput = (argc >= 4 && std::string(argv[3]) == "--json");

        if (!std::filesystem::exists(filePath))
        {
            std::cerr << "Error: File does not exist: " << filePath << "\n";
            return 1;
        }

        // 1. Inspect format
        Core::InspectionResult inspection = Core::FileInspector::inspect(filePath);

        // 2. Read file
        std::ifstream file(filePath, std::ios::binary);
        file.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> buffer(fileSize);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

        // 3. Chunk file
        auto chunks = Core::AsymmetricChunker::chunkBuffer(buffer.data(), buffer.size(), inspection);

        // 4. Test codec on each chunk
        struct ChunkCodecInfo
        {
            size_t index;
            Core::StructuralChunk chunk;
            Core::CodecResult codec;
        };

        std::vector<ChunkCodecInfo> results;
        size_t totalRaw = 0;
        size_t totalCompressed = 0;

        for (size_t i = 0; i < chunks.size(); ++i)
        {
            const auto& ch = chunks[i];
            const uint8_t* chunkData = buffer.data() + ch.offset;
            auto res = Core::PolymorphicCodec::compressChunk(chunkData, ch.length, ch.category, inspection.format);
            totalRaw += res.rawSize;
            totalCompressed += res.compressedSize;
            results.push_back({i, ch, res});
        }

        if (jsonOutput)
        {
            std::cout << "{\n";
            std::cout << "  \"file\": \"" << escapeJson(filePath) << "\",\n";
            std::cout << "  \"format\": \"" << inspection.formatName << "\",\n";
            std::cout << "  \"raw_size\": " << totalRaw << ",\n";
            std::cout << "  \"compressed_size\": " << totalCompressed << ",\n";
            std::cout << "  \"total_ratio\": " << (totalCompressed > 0 ? static_cast<double>(totalRaw) / totalCompressed : 1.0) << ",\n";
            std::cout << "  \"chunks\": [\n";

            for (size_t i = 0; i < results.size(); ++i)
            {
                const auto& item = results[i];
                std::cout << "    {\n";
                std::cout << "      \"index\": " << item.index << ",\n";
                std::cout << "      \"offset\": " << item.chunk.offset << ",\n";
                std::cout << "      \"raw_size\": " << item.codec.rawSize << ",\n";
                std::cout << "      \"compressed_size\": " << item.codec.compressedSize << ",\n";
                std::cout << "      \"ratio\": " << item.codec.compressionRatio << ",\n";
                std::cout << "      \"codec_type\": \"" << Core::PolymorphicCodec::codecTypeToString(item.codec.codecUsed) << "\",\n";
                std::cout << "      \"codec_method\": \"" << item.codec.codecMethod << "\",\n";
                std::cout << "      \"raw_sha256\": \"" << item.codec.rawSha256 << "\"\n";
                std::cout << "    }" << (i + 1 < results.size() ? "," : "") << "\n";
            }

            std::cout << "  ]\n";
            std::cout << "}\n";
            return 0;
        }

        std::cout << "\n";
        std::cout << "  🦎 AI-GIT POLYMORPHIC CODEC INSPECTION\n";
        std::cout << "  ─────────────────────────────────────────────────────────────────────────────────────────────\n";
        std::cout << "  │ Target File   : " << filePath << "\n";
        std::cout << "  │ Format        : " << inspection.formatName << " (" << Core::FileInspector::categoryToString(inspection.category) << ")\n";
        std::cout << "  │ Total Raw     : " << formatBytes(totalRaw) << "\n";
        std::cout << "  │ Compressed    : " << formatBytes(totalCompressed) << "\n";
        std::cout << "  │ Space Savings : " << std::fixed << std::setprecision(1)
                  << (totalRaw > 0 ? (1.0 - (static_cast<double>(totalCompressed) / totalRaw)) * 100.0 : 0.0) << "%\n";
        std::cout << "  ─────────────────────────────────────────────────────────────────────────────────────────────\n";
        std::cout << "  " << std::left << std::setw(6)  << "CHUNK"
                  << std::left << std::setw(10) << "RAW"
                  << std::left << std::setw(12) << "COMPRESSED"
                  << std::left << std::setw(8)  << "RATIO"
                  << std::left << std::setw(38) << "CODEC METHOD"
                  << "CAS SHA-256 (RAW)\n";
        std::cout << "  ─────────────────────────────────────────────────────────────────────────────────────────────\n";

        for (const auto& item : results)
        {
            std::ostringstream ratioStr;
            ratioStr << std::fixed << std::setprecision(1) << item.codec.compressionRatio << "x";
            std::cout << "  "
                      << "#" << std::left << std::setw(5) << (item.index + 1)
                      << std::left << std::setw(10) << formatBytes(item.codec.rawSize)
                      << std::left << std::setw(12) << formatBytes(item.codec.compressedSize)
                      << std::left << std::setw(8)  << ratioStr.str()
                      << std::left << std::setw(38) << item.codec.codecMethod
                      << item.codec.rawSha256.substr(0, 16) << "...\n";
        }
        std::cout << "  ─────────────────────────────────────────────────────────────────────────────────────────────\n";
        std::cout << "  ✔ Bit-Exact Decoupling: CAS hashes are calculated on raw bytes before compression.\n\n";

        return 0;
    }

    // ------------------------------------------------------------------------
    // COMMAND: compress
    // ------------------------------------------------------------------------
    if (command == "compress")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: aigit-codec compress <input_file> <output_archive>\n";
            return 1;
        }

        std::string inputFile = argv[2];
        std::string outputFile = argv[3];

        Core::InspectionResult inspection = Core::FileInspector::inspect(inputFile);

        std::ifstream in(inputFile, std::ios::binary);
        if (!in.is_open())
        {
            std::cerr << "Error: Failed to open input file: " << inputFile << "\n";
            return 1;
        }

        in.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(in.tellg());
        in.seekg(0, std::ios::beg);
        std::vector<uint8_t> buffer(size);
        in.read(reinterpret_cast<char*>(buffer.data()), size);

        Core::ChunkCategory category = Core::ChunkCategory::GenericData;
        if (inspection.format == Core::FileFormat::PNG ||
            inspection.format == Core::FileFormat::JPEG ||
            inspection.format == Core::FileFormat::MP4)
        {
            category = Core::ChunkCategory::MediaPayload;
        }
        else if (inspection.format == Core::FileFormat::SafeTensors ||
                 inspection.format == Core::FileFormat::GGUF)
        {
            category = Core::ChunkCategory::TensorPayload;
        }
        else if (inspection.format == Core::FileFormat::Text)
        {
            category = Core::ChunkCategory::MetadataHeader;
        }

        auto res = Core::PolymorphicCodec::compressChunk(buffer.data(), buffer.size(), category, inspection.format);

        std::ofstream out(outputFile, std::ios::binary);
        out.write(reinterpret_cast<const char*>(res.payload.data()), res.payload.size());

        std::cout << "✔ Compressed " << inputFile << " (" << formatBytes(size) << ") ➔ " << outputFile << " (" << formatBytes(res.payload.size()) << ")\n";
        std::cout << "  Method   : " << res.codecMethod << "\n";
        std::cout << "  Raw CAS  : " << res.rawSha256 << "\n";
        std::cout << "  Ratio    : " << std::fixed << std::setprecision(2) << res.compressionRatio << "x\n";

        return 0;
    }

    // ------------------------------------------------------------------------
    // COMMAND: decompress
    // ------------------------------------------------------------------------
    if (command == "decompress")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: aigit-codec decompress <input_archive> <output_file>\n";
            return 1;
        }

        std::string inputFile = argv[2];
        std::string outputFile = argv[3];

        std::ifstream in(inputFile, std::ios::binary);
        if (!in.is_open())
        {
            std::cerr << "Error: Failed to open archive: " << inputFile << "\n";
            return 1;
        }

        in.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(in.tellg());
        in.seekg(0, std::ios::beg);
        std::vector<uint8_t> buffer(size);
        in.read(reinterpret_cast<char*>(buffer.data()), size);

        std::string restoredSha256;
        std::string methodUsed;

        try
        {
            auto restored = Core::PolymorphicCodec::decompressContainer(buffer.data(), buffer.size(), &restoredSha256, &methodUsed);

            std::ofstream out(outputFile, std::ios::binary);
            out.write(reinterpret_cast<const char*>(restored.data()), restored.size());

            std::cout << "✔ Decompressed " << inputFile << " ➔ " << outputFile << " (" << formatBytes(restored.size()) << ")\n";
            std::cout << "  Method       : " << methodUsed << "\n";
            std::cout << "  Verified CAS : " << restoredSha256 << " [BIT-EXACT MATCH]\n";
            return 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << "❌ Decompression Error: " << e.what() << "\n";
            return 1;
        }
    }

    printHelp();
    return 1;
}
