#include "../diffing/structural_differ.hpp"
#include "../diffing/metadata_harvester.hpp"
#include "../inspection/file_inspector.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

static void printHelp()
{
    std::cout << "\n  🦎 AI-GIT SEMANTIC STRUCTURAL DIFFER & METADATA HARVESTER\n";
    std::cout << "  ─────────────────────────────────────────────────────────────────────────────\n";
    std::cout << "  USAGE:\n";
    std::cout << "    aigit-diff inspect <model_file> [--json]          Inspect layers, shapes & dtypes\n";
    std::cout << "    aigit-diff diff <fileA> <fileB> [--json]          Semantic layer-by-layer diff\n";
    std::cout << "    aigit-diff harvest <file> <sqlite_db>             Index tensors into SQLite database\n\n";
    std::cout << "  FORMAT-SPECIFIC METHODS:\n";
    std::cout << "    • SafeTensors  ➔ harvestSafeTensorsMetadata / diffSafeTensorsModels\n";
    std::cout << "    • GGUF         ➔ harvestGGUFMetadata / diffGGUFModels\n";
    std::cout << "    • Parquet      ➔ harvestParquetMetadata / diffParquetDatasets\n";
    std::cout << "    • Config/JSON  ➔ harvestJsonConfigMetadata / diffJsonConfigs\n\n";
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

        std::string targetFile = argv[2];
        bool jsonOutput = (argc >= 4 && std::string(argv[3]) == "--json");

        if (!std::filesystem::exists(targetFile))
        {
            std::cerr << "Error: File does not exist: " << targetFile << "\n";
            return 1;
        }

        Core::ModelMetadata model;
        if (Core::MetadataHarvester::harvestSafeTensorsMetadata(targetFile, model))
        {
            if (jsonOutput)
            {
                std::cout << "{\n";
                std::cout << "  \"file\": \"" << escapeJson(targetFile) << "\",\n";
                std::cout << "  \"format\": \"" << model.formatName << "\",\n";
                std::cout << "  \"total_parameters\": " << model.totalParameters << ",\n";
                std::cout << "  \"tensors\": [\n";
                for (size_t i = 0; i < model.tensors.size(); ++i)
                {
                    const auto& t = model.tensors[i];
                    std::cout << "    {\n";
                    std::cout << "      \"name\": \"" << escapeJson(t.name) << "\",\n";
                    std::cout << "      \"dtype\": \"" << t.dtype << "\",\n";
                    std::cout << "      \"shape\": \"" << Core::MetadataHarvester::shapeToString(t.shape) << "\",\n";
                    std::cout << "      \"elements\": " << t.numElements << ",\n";
                    std::cout << "      \"hash\": \"" << t.tensorHash << "\"\n";
                    std::cout << "    }" << (i + 1 < model.tensors.size() ? "," : "") << "\n";
                }
                std::cout << "  ]\n";
                std::cout << "}\n";
                return 0;
            }

            std::cout << "\n";
            std::cout << "  🦎 AI-GIT MODEL METADATA INSPECTOR\n";
            std::cout << "  ─────────────────────────────────────────────────────────────────────────────\n";
            std::cout << "  │ Model File   : " << targetFile << "\n";
            std::cout << "  │ Format       : " << model.formatName << "\n";
            std::cout << "  │ Parameters   : " << model.totalParameters << " parameters\n";
            std::cout << "  │ Total Layers : " << model.tensors.size() << " tensors\n";
            if (!model.attributes.empty())
            {
                std::cout << "  │ Attributes   :\n";
                for (const auto& kv : model.attributes)
                {
                    std::cout << "  │   • " << kv.first << ": " << kv.second << "\n";
                }
            }
            std::cout << "  ─────────────────────────────────────────────────────────────────────────────\n";
            std::cout << "  " << std::left << std::setw(30) << "TENSOR LAYER"
                      << std::left << std::setw(8)  << "DTYPE"
                      << std::left << std::setw(16) << "SHAPE"
                      << std::left << std::setw(12) << "ELEMENTS"
                      << "WEIGHT HASH\n";
            std::cout << "  ─────────────────────────────────────────────────────────────────────────────\n";

            for (const auto& t : model.tensors)
            {
                std::cout << "  "
                          << std::left << std::setw(30) << t.name
                          << std::left << std::setw(8)  << t.dtype
                          << std::left << std::setw(16) << Core::MetadataHarvester::shapeToString(t.shape)
                          << std::left << std::setw(12) << t.numElements
                          << t.tensorHash.substr(0, 16) << "...\n";
            }
            std::cout << "  ─────────────────────────────────────────────────────────────────────────────\n\n";
            return 0;
        }

        std::cerr << "Error: File format not supported for model inspection: " << targetFile << "\n";
        return 1;
    }

    // ------------------------------------------------------------------------
    // COMMAND: diff
    // ------------------------------------------------------------------------
    if (command == "diff")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: aigit-diff diff <fileA> <fileB> [--json]\n";
            return 1;
        }

        std::string fileA = argv[2];
        std::string fileB = argv[3];
        bool jsonOutput = (argc >= 5 && std::string(argv[4]) == "--json");

        if (!std::filesystem::exists(fileA) || !std::filesystem::exists(fileB))
        {
            std::cerr << "Error: Both files must exist to perform diff.\n";
            return 1;
        }

        auto semDiff = Core::StructuralDiffer::diffFiles(fileA, fileB);

        if (jsonOutput)
        {
            std::cout << "{\n";
            std::cout << "  \"fileA\": \"" << escapeJson(fileA) << "\",\n";
            std::cout << "  \"fileB\": \"" << escapeJson(fileB) << "\",\n";
            std::cout << "  \"diff_method\": \"" << semDiff.diffMethod << "\",\n";

            if (semDiff.hasModelDiff)
            {
                const auto& md = semDiff.modelDiff;
                std::cout << "  \"identical_layers\": " << md.identicalLayers << ",\n";
                std::cout << "  \"modified_layers\": " << md.modifiedLayers << ",\n";
                std::cout << "  \"added_layers\": " << md.addedLayers << ",\n";
                std::cout << "  \"removed_layers\": " << md.removedLayers << ",\n";
                std::cout << "  \"param_delta\": " << md.totalParamDelta << ",\n";
                std::cout << "  \"layer_diffs\": [\n";

                for (size_t i = 0; i < md.layerDiffs.size(); ++i)
                {
                    const auto& ld = md.layerDiffs[i];
                    std::cout << "    {\n";
                    std::cout << "      \"name\": \"" << escapeJson(ld.tensorName) << "\",\n";
                    std::cout << "      \"status\": \"" << Core::StructuralDiffer::layerDiffTypeToString(ld.diffType) << "\",\n";
                    std::cout << "      \"old_shape\": \"" << ld.oldShape << "\",\n";
                    std::cout << "      \"new_shape\": \"" << ld.newShape << "\",\n";
                    std::cout << "      \"param_delta\": " << ld.parameterDelta << "\n";
                    std::cout << "    }" << (i + 1 < md.layerDiffs.size() ? "," : "") << "\n";
                }
                std::cout << "  ]\n";
            }
            std::cout << "}\n";
            return 0;
        }

        std::cout << "\n";
        std::cout << "  🦎 AI-GIT SEMANTIC MODEL DIFF\n";
        std::cout << "  ─────────────────────────────────────────────────────────────────────────────\n";
        std::cout << "  │ Version A     : " << fileA << "\n";
        std::cout << "  │ Version B     : " << fileB << "\n";
        std::cout << "  │ Diff Method   : " << semDiff.diffMethod << "()\n";

        if (semDiff.hasModelDiff)
        {
            const auto& md = semDiff.modelDiff;
            std::cout << "  │ Param Delta   : " << (md.totalParamDelta >= 0 ? "+" : "") << md.totalParamDelta << " parameters\n";
            std::cout << "  │ Layer Summary : " << md.identicalLayers << " identical (100% deduplicated), "
                      << md.modifiedLayers << " modified, "
                      << md.addedLayers << " added, "
                      << md.removedLayers << " removed\n";

            if (!md.attributeDiffs.empty())
            {
                std::cout << "  │ Hyperparameter Updates:\n";
                for (const auto& ad : md.attributeDiffs)
                {
                    std::cout << "  │   • " << ad.key << ": \"" << ad.oldValue << "\" ➔ \"" << ad.newValue << "\"\n";
                }
            }

            std::cout << "  ─────────────────────────────────────────────────────────────────────────────\n";
            std::cout << "  " << std::left << std::setw(18) << "DIFF STATUS"
                      << std::left << std::setw(32) << "TENSOR LAYER"
                      << std::left << std::setw(16) << "OLD SHAPE"
                      << "NEW SHAPE\n";
            std::cout << "  ─────────────────────────────────────────────────────────────────────────────\n";

            for (const auto& ld : md.layerDiffs)
            {
                std::cout << "  "
                          << std::left << std::setw(18) << Core::StructuralDiffer::layerDiffTypeToString(ld.diffType)
                          << std::left << std::setw(32) << ld.tensorName
                          << std::left << std::setw(16) << ld.oldShape
                          << ld.newShape << "\n";
            }
            std::cout << "  ─────────────────────────────────────────────────────────────────────────────\n\n";
            return 0;
        }

        std::cout << "  No structural model diff available for this format.\n";
        return 0;
    }

    // ------------------------------------------------------------------------
    // COMMAND: harvest
    // ------------------------------------------------------------------------
    if (command == "harvest")
    {
        if (argc < 4)
        {
            std::cerr << "Usage: aigit-diff harvest <model_file> <sqlite_db_path>\n";
            return 1;
        }

        std::string modelFile = argv[2];
        std::string dbPath = argv[3];

        Core::ModelMetadata model;
        if (!Core::MetadataHarvester::harvestSafeTensorsMetadata(modelFile, model))
        {
            std::cerr << "Error: Failed to harvest metadata from: " << modelFile << "\n";
            return 1;
        }

        sqlite3* db = nullptr;
        if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK)
        {
            std::cerr << "Error: Failed to open SQLite DB: " << dbPath << "\n";
            return 1;
        }

        std::string modelId = std::filesystem::path(modelFile).stem().string();
        if (Core::MetadataHarvester::persistModelToSQLite(db, modelId, model))
        {
            std::cout << "✔ Successfully harvested " << model.tensors.size() << " tensors and "
                      << model.attributes.size() << " attributes into SQLite DB: " << dbPath << "\n";
            sqlite3_close(db);
            return 0;
        }

        sqlite3_close(db);
        std::cerr << "Error: SQLite persistence failed.\n";
        return 1;
    }

    printHelp();
    return 1;
}
