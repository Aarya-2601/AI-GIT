#pragma once

#include "metadata_harvester.hpp"
#include <string>
#include <vector>
#include <map>

namespace Core
{

enum class LayerDiffType
{
    Added,           // Layer exists in new model, missing in old
    Removed,         // Layer exists in old model, missing in new
    ModifiedShape,   // Dimension change (e.g. fine-tuned with new heads/vocab)
    ModifiedDtype,   // Precision change (e.g. F32 -> BF16)
    WeightsModified, // Same name, shape, dtype, but weights changed
    WeightsIdentical // Exactly identical weights (100% deduplicated in CAS!)
};

struct LayerDiff
{
    std::string tensorName;
    LayerDiffType diffType;
    std::string oldShape;
    std::string newShape;
    std::string oldDtype;
    std::string newDtype;
    std::string oldHash;
    std::string newHash;
    int64_t parameterDelta = 0;
};

struct AttributeDiff
{
    std::string key;
    std::string oldValue;
    std::string newValue;
    bool isAdded = false;
    bool isRemoved = false;
};

struct ModelDiffResult
{
    std::string diffMethod; // "diffSafeTensorsModels", "diffGGUFModels", etc.
    size_t identicalLayers = 0;
    size_t modifiedLayers = 0;
    size_t addedLayers = 0;
    size_t removedLayers = 0;
    int64_t totalParamDelta = 0;
    std::vector<AttributeDiff> attributeDiffs;
    std::vector<LayerDiff> layerDiffs;
};

struct DatasetDiffResult
{
    std::string diffMethod; // "diffParquetDatasets"
    int64_t rowDelta = 0;
    std::vector<std::string> addedColumns;
    std::vector<std::string> removedColumns;
    std::vector<std::string> modifiedColumns;
};

struct JsonDiffResult
{
    std::string diffMethod; // "diffJsonConfigs"
    std::vector<AttributeDiff> diffs;
};

struct SemanticDiffResult
{
    FileFormat formatA = FileFormat::Unknown;
    FileFormat formatB = FileFormat::Unknown;
    std::string formatName;
    std::string diffMethod;
    bool isBinaryIdentical = false;
    bool hasModelDiff = false;
    bool hasDatasetDiff = false;
    bool hasJsonDiff = false;
    ModelDiffResult modelDiff;
    DatasetDiffResult datasetDiff;
    JsonDiffResult jsonDiff;
};

class StructuralDiffer
{
public:
    // Explicit format-specific diffing methods
    static ModelDiffResult diffSafeTensorsModels(
        const ModelMetadata& oldModel,
        const ModelMetadata& newModel
    );

    static ModelDiffResult diffGGUFModels(
        const ModelMetadata& oldModel,
        const ModelMetadata& newModel
    );

    static DatasetDiffResult diffParquetDatasets(
        const DatasetMetadata& oldDs,
        const DatasetMetadata& newDs
    );

    static JsonDiffResult diffJsonConfigs(
        const std::map<std::string, std::string>& oldJson,
        const std::map<std::string, std::string>& newJson
    );

    // Universal file comparison that chooses the appropriate semantic differ
    static SemanticDiffResult diffFiles(
        const std::filesystem::path& fileA,
        const std::filesystem::path& fileB
    );

    static const char* layerDiffTypeToString(LayerDiffType type);
};

} // namespace Core
