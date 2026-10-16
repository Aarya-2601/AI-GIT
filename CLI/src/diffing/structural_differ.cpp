#include "structural_differ.hpp"
#include "../hashing/hashing.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>

namespace Core
{

const char* StructuralDiffer::layerDiffTypeToString(LayerDiffType type)
{
    switch (type)
    {
        case LayerDiffType::Added:            return "ADDED";
        case LayerDiffType::Removed:          return "REMOVED";
        case LayerDiffType::ModifiedShape:    return "SHAPE_CHANGED";
        case LayerDiffType::ModifiedDtype:    return "DTYPE_CHANGED";
        case LayerDiffType::WeightsModified:  return "WEIGHTS_MODIFIED";
        case LayerDiffType::WeightsIdentical: return "IDENTICAL";
        default:                              return "UNKNOWN";
    }
}

ModelDiffResult StructuralDiffer::diffSafeTensorsModels(
    const ModelMetadata& oldModel,
    const ModelMetadata& newModel
)
{
    ModelDiffResult result;
    result.diffMethod = "diffSafeTensorsModels";
    result.totalParamDelta = static_cast<int64_t>(newModel.totalParameters) - static_cast<int64_t>(oldModel.totalParameters);

    // 1. Diff Attributes / Hyperparameters
    std::set<std::string> allAttrKeys;
    for (const auto& kv : oldModel.attributes) allAttrKeys.insert(kv.first);
    for (const auto& kv : newModel.attributes) allAttrKeys.insert(kv.first);

    for (const auto& key : allAttrKeys)
    {
        auto itOld = oldModel.attributes.find(key);
        auto itNew = newModel.attributes.find(key);

        if (itOld == oldModel.attributes.end())
        {
            result.attributeDiffs.push_back({key, "", itNew->second, true, false});
        }
        else if (itNew == newModel.attributes.end())
        {
            result.attributeDiffs.push_back({key, itOld->second, "", false, true});
        }
        else if (itOld->second != itNew->second)
        {
            result.attributeDiffs.push_back({key, itOld->second, itNew->second, false, false});
        }
    }

    // 2. Diff Tensor Layers
    std::map<std::string, const TensorInfo*> oldTensors;
    for (const auto& t : oldModel.tensors) oldTensors[t.name] = &t;

    std::map<std::string, const TensorInfo*> newTensors;
    for (const auto& t : newModel.tensors) newTensors[t.name] = &t;

    std::set<std::string> allTensorNames;
    for (const auto& kv : oldTensors) allTensorNames.insert(kv.first);
    for (const auto& kv : newTensors) allTensorNames.insert(kv.first);

    for (const auto& name : allTensorNames)
    {
        auto itOld = oldTensors.find(name);
        auto itNew = newTensors.find(name);

        if (itOld == oldTensors.end())
        {
            // Added layer
            const auto* nt = itNew->second;
            result.addedLayers++;
            result.layerDiffs.push_back({
                name,
                LayerDiffType::Added,
                "",
                MetadataHarvester::shapeToString(nt->shape),
                "",
                nt->dtype,
                "",
                nt->tensorHash,
                static_cast<int64_t>(nt->numElements)
            });
        }
        else if (itNew == newTensors.end())
        {
            // Removed layer
            const auto* ot = itOld->second;
            result.removedLayers++;
            result.layerDiffs.push_back({
                name,
                LayerDiffType::Removed,
                MetadataHarvester::shapeToString(ot->shape),
                "",
                ot->dtype,
                "",
                ot->tensorHash,
                "",
                -static_cast<int64_t>(ot->numElements)
            });
        }
        else
        {
            // Layer present in both
            const auto* ot = itOld->second;
            const auto* nt = itNew->second;

            if (ot->shape != nt->shape)
            {
                result.modifiedLayers++;
                result.layerDiffs.push_back({
                    name,
                    LayerDiffType::ModifiedShape,
                    MetadataHarvester::shapeToString(ot->shape),
                    MetadataHarvester::shapeToString(nt->shape),
                    ot->dtype,
                    nt->dtype,
                    ot->tensorHash,
                    nt->tensorHash,
                    static_cast<int64_t>(nt->numElements) - static_cast<int64_t>(ot->numElements)
                });
            }
            else if (ot->dtype != nt->dtype)
            {
                result.modifiedLayers++;
                result.layerDiffs.push_back({
                    name,
                    LayerDiffType::ModifiedDtype,
                    MetadataHarvester::shapeToString(ot->shape),
                    MetadataHarvester::shapeToString(nt->shape),
                    ot->dtype,
                    nt->dtype,
                    ot->tensorHash,
                    nt->tensorHash,
                    0
                });
            }
            else if (ot->tensorHash != nt->tensorHash)
            {
                result.modifiedLayers++;
                result.layerDiffs.push_back({
                    name,
                    LayerDiffType::WeightsModified,
                    MetadataHarvester::shapeToString(ot->shape),
                    MetadataHarvester::shapeToString(nt->shape),
                    ot->dtype,
                    nt->dtype,
                    ot->tensorHash,
                    nt->tensorHash,
                    0
                });
            }
            else
            {
                result.identicalLayers++;
                result.layerDiffs.push_back({
                    name,
                    LayerDiffType::WeightsIdentical,
                    MetadataHarvester::shapeToString(ot->shape),
                    MetadataHarvester::shapeToString(nt->shape),
                    ot->dtype,
                    nt->dtype,
                    ot->tensorHash,
                    nt->tensorHash,
                    0
                });
            }
        }
    }

    return result;
}

ModelDiffResult StructuralDiffer::diffGGUFModels(
    const ModelMetadata& oldModel,
    const ModelMetadata& newModel
)
{
    ModelDiffResult result;
    result.diffMethod = "diffGGUFModels";

    for (const auto& kv : oldModel.attributes)
    {
        auto it = newModel.attributes.find(kv.first);
        if (it != newModel.attributes.end() && it->second != kv.second)
        {
            result.attributeDiffs.push_back({kv.first, kv.second, it->second, false, false});
        }
    }

    return result;
}

DatasetDiffResult StructuralDiffer::diffParquetDatasets(
    const DatasetMetadata& oldDs,
    const DatasetMetadata& newDs
)
{
    DatasetDiffResult result;
    result.diffMethod = "diffParquetDatasets";
    result.rowDelta = newDs.totalRows - oldDs.totalRows;

    std::map<std::string, std::string> oldCols, newCols;
    for (const auto& c : oldDs.columns) oldCols[c.name] = c.physicalType;
    for (const auto& c : newDs.columns) newCols[c.name] = c.physicalType;

    for (const auto& nc : newCols)
    {
        if (oldCols.find(nc.first) == oldCols.end())
        {
            result.addedColumns.push_back(nc.first + " (" + nc.second + ")");
        }
    }

    for (const auto& oc : oldCols)
    {
        if (newCols.find(oc.first) == newCols.end())
        {
            result.removedColumns.push_back(oc.first + " (" + oc.second + ")");
        }
        else if (newCols[oc.first] != oc.second)
        {
            result.modifiedColumns.push_back(oc.first + " (" + oc.second + " ➔ " + newCols[oc.first] + ")");
        }
    }

    return result;
}

JsonDiffResult StructuralDiffer::diffJsonConfigs(
    const std::map<std::string, std::string>& oldJson,
    const std::map<std::string, std::string>& newJson
)
{
    JsonDiffResult result;
    result.diffMethod = "diffJsonConfigs";

    std::set<std::string> allKeys;
    for (const auto& kv : oldJson) allKeys.insert(kv.first);
    for (const auto& kv : newJson) allKeys.insert(kv.first);

    for (const auto& k : allKeys)
    {
        auto itOld = oldJson.find(k);
        auto itNew = newJson.find(k);

        if (itOld == oldJson.end())
        {
            result.diffs.push_back({k, "", itNew->second, true, false});
        }
        else if (itNew == newJson.end())
        {
            result.diffs.push_back({k, itOld->second, "", false, true});
        }
        else if (itOld->second != itNew->second)
        {
            result.diffs.push_back({k, itOld->second, itNew->second, false, false});
        }
    }

    return result;
}

SemanticDiffResult StructuralDiffer::diffFiles(
    const std::filesystem::path& fileA,
    const std::filesystem::path& fileB
)
{
    SemanticDiffResult res;

    auto inspA = FileInspector::inspect(fileA);
    auto inspB = FileInspector::inspect(fileB);

    res.formatA = inspA.format;
    res.formatB = inspB.format;
    res.formatName = inspA.formatName;

    if (inspA.format == FileFormat::SafeTensors && inspB.format == FileFormat::SafeTensors)
    {
        ModelMetadata metaA, metaB;
        if (MetadataHarvester::harvestSafeTensorsMetadata(fileA, metaA) &&
            MetadataHarvester::harvestSafeTensorsMetadata(fileB, metaB))
        {
            res.hasModelDiff = true;
            res.diffMethod = "diffSafeTensorsModels";
            res.modelDiff = diffSafeTensorsModels(metaA, metaB);
            return res;
        }
    }

    if (inspA.format == FileFormat::GGUF && inspB.format == FileFormat::GGUF)
    {
        ModelMetadata metaA, metaB;
        if (MetadataHarvester::harvestGGUFMetadata(fileA, metaA) &&
            MetadataHarvester::harvestGGUFMetadata(fileB, metaB))
        {
            res.hasModelDiff = true;
            res.diffMethod = "diffGGUFModels";
            res.modelDiff = diffGGUFModels(metaA, metaB);
            return res;
        }
    }

    if (inspA.format == FileFormat::Parquet && inspB.format == FileFormat::Parquet)
    {
        DatasetMetadata dsA, dsB;
        if (MetadataHarvester::harvestParquetMetadata(fileA, dsA) &&
            MetadataHarvester::harvestParquetMetadata(fileB, dsB))
        {
            res.hasDatasetDiff = true;
            res.diffMethod = "diffParquetDatasets";
            res.datasetDiff = diffParquetDatasets(dsA, dsB);
            return res;
        }
    }

    // JSON / Config fallback
    if (fileA.extension() == ".json" || fileB.extension() == ".json" || inspA.format == FileFormat::Text)
    {
        std::map<std::string, std::string> mapA, mapB;
        if (MetadataHarvester::harvestJsonConfigMetadata(fileA, mapA) &&
            MetadataHarvester::harvestJsonConfigMetadata(fileB, mapB))
        {
            res.hasJsonDiff = true;
            res.diffMethod = "diffJsonConfigs";
            res.jsonDiff = diffJsonConfigs(mapA, mapB);
            return res;
        }
    }

    res.diffMethod = "diffGenericBinary";
    return res;
}

} // namespace Core
