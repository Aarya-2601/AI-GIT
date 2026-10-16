#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include <sqlite3.h>

#include "../inspection/file_inspector.hpp"

namespace Core
{

struct TensorInfo
{
    std::string name;              // e.g. "model.layers.0.weight"
    std::string dtype;             // e.g. "F32", "BF16", "F16"
    std::vector<int64_t> shape;    // e.g. [1024, 768]
    size_t numElements = 0;        // e.g. 786432
    size_t dataOffset = 0;         // Byte offset in file
    size_t byteLength = 0;         // Size in bytes
    std::string tensorHash;        // SHA-256 of the uncompressed tensor weights
};

struct ModelMetadata
{
    FileFormat format = FileFormat::Unknown;
    std::string formatName;
    std::filesystem::path filePath;
    size_t fileSize = 0;
    size_t totalParameters = 0;
    std::map<std::string, std::string> attributes; // hyperparameters, author, architecture, etc.
    std::vector<TensorInfo> tensors;
};

struct ColumnInfo
{
    std::string name;
    std::string physicalType;      // INT64, FLOAT, BYTE_ARRAY, etc.
    int64_t numValues = 0;
};

struct DatasetMetadata
{
    FileFormat format = FileFormat::Parquet;
    std::filesystem::path filePath;
    size_t fileSize = 0;
    int64_t totalRows = 0;
    size_t rowGroups = 0;
    std::vector<ColumnInfo> columns;
    std::map<std::string, std::string> keyValues;
};

class MetadataHarvester
{
public:
    // Explicit format-specific harvesting methods
    static bool harvestSafeTensorsMetadata(
        const std::filesystem::path& path,
        ModelMetadata& out
    );

    static bool harvestGGUFMetadata(
        const std::filesystem::path& path,
        ModelMetadata& out
    );

    static bool harvestParquetMetadata(
        const std::filesystem::path& path,
        DatasetMetadata& out
    );

    static bool harvestJsonConfigMetadata(
        const std::filesystem::path& path,
        std::map<std::string, std::string>& out
    );

    // SQLite Indexing & Queries
    static bool persistModelToSQLite(
        sqlite3* db,
        const std::string& modelId,
        const ModelMetadata& meta
    );

    static bool persistDatasetToSQLite(
        sqlite3* db,
        const std::string& datasetId,
        const DatasetMetadata& meta
    );

    static std::string shapeToString(const std::vector<int64_t>& shape);
};

} // namespace Core
