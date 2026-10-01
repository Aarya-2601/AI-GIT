#include "metadata_harvester.hpp"
#include "../hashing/hashing.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <chrono>
#include <algorithm>

namespace Core
{

std::string MetadataHarvester::shapeToString(const std::vector<int64_t>& shape)
{
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < shape.size(); ++i)
    {
        ss << shape[i];
        if (i + 1 < shape.size()) ss << ", ";
    }
    ss << "]";
    return ss.str();
}

// ----------------------------------------------------------------------------
// Lightweight JSON Tokenizer & Parser for SafeTensors & Config Metadata
// ----------------------------------------------------------------------------

static void skipWhitespace(const std::string& str, size_t& pos)
{
    while (pos < str.size() && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\r' || str[pos] == '\n'))
    {
        pos++;
    }
}

static std::string parseStringToken(const std::string& str, size_t& pos)
{
    skipWhitespace(str, pos);
    if (pos >= str.size() || str[pos] != '"') return "";
    pos++; // skip leading quote

    std::string val;
    while (pos < str.size())
    {
        char c = str[pos++];
        if (c == '"')
        {
            return val;
        }
        if (c == '\\' && pos < str.size())
        {
            char escaped = str[pos++];
            if (escaped == '"') val += '"';
            else if (escaped == '\\') val += '\\';
            else if (escaped == 'n') val += '\n';
            else if (escaped == 't') val += '\t';
            else val += escaped;
        }
        else
        {
            val += c;
        }
    }
    return val;
}

static std::vector<int64_t> parseIntArray(const std::string& str, size_t& pos)
{
    skipWhitespace(str, pos);
    if (pos >= str.size() || str[pos] != '[') return {};
    pos++; // skip '['

    std::vector<int64_t> arr;
    while (pos < str.size())
    {
        skipWhitespace(str, pos);
        if (pos >= str.size()) break;
        if (str[pos] == ']')
        {
            pos++;
            break;
        }
        if (str[pos] == ',')
        {
            pos++;
            continue;
        }

        size_t start = pos;
        while (pos < str.size() && (std::isdigit(str[pos]) || str[pos] == '-'))
        {
            pos++;
        }
        if (pos > start)
        {
            try
            {
                arr.push_back(std::stoll(str.substr(start, pos - start)));
            }
            catch (...) {}
        }
    }
    return arr;
}

// ----------------------------------------------------------------------------
// Format-Specific Harvesting: SafeTensors
// ----------------------------------------------------------------------------

bool MetadataHarvester::harvestSafeTensorsMetadata(
    const std::filesystem::path& path,
    ModelMetadata& out
)
{
    if (!std::filesystem::exists(path)) return false;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(0, std::ios::end);
    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    if (fileSize < 10) return false;

    uint8_t lenBuf[8];
    file.read(reinterpret_cast<char*>(lenBuf), 8);

    uint64_t headerLen = static_cast<uint64_t>(lenBuf[0])
                       | (static_cast<uint64_t>(lenBuf[1]) << 8)
                       | (static_cast<uint64_t>(lenBuf[2]) << 16)
                       | (static_cast<uint64_t>(lenBuf[3]) << 24)
                       | (static_cast<uint64_t>(lenBuf[4]) << 32)
                       | (static_cast<uint64_t>(lenBuf[5]) << 40)
                       | (static_cast<uint64_t>(lenBuf[6]) << 48)
                       | (static_cast<uint64_t>(lenBuf[7]) << 56);

    if (headerLen == 0 || headerLen > fileSize - 8) return false;

    std::string jsonHeader(headerLen, '\0');
    file.read(&jsonHeader[0], headerLen);

    out.format = FileFormat::SafeTensors;
    out.formatName = "SafeTensors";
    out.filePath = path;
    out.fileSize = fileSize;
    out.totalParameters = 0;
    out.attributes.clear();
    out.tensors.clear();

    size_t payloadBase = 8 + headerLen;

    // Parse SafeTensors JSON header
    size_t pos = 0;
    skipWhitespace(jsonHeader, pos);
    if (pos >= jsonHeader.size() || jsonHeader[pos] != '{') return false;
    pos++;

    while (pos < jsonHeader.size())
    {
        skipWhitespace(jsonHeader, pos);
        if (pos >= jsonHeader.size() || jsonHeader[pos] == '}') break;
        if (jsonHeader[pos] == ',') { pos++; continue; }

        std::string key = parseStringToken(jsonHeader, pos);
        if (key.empty()) break;

        skipWhitespace(jsonHeader, pos);
        if (pos >= jsonHeader.size() || jsonHeader[pos] != ':') break;
        pos++; // skip ':'

        skipWhitespace(jsonHeader, pos);
        if (pos >= jsonHeader.size()) break;

        // Check if value is __metadata__ object
        if (key == "__metadata__")
        {
            if (jsonHeader[pos] == '{')
            {
                pos++;
                while (pos < jsonHeader.size())
                {
                    skipWhitespace(jsonHeader, pos);
                    if (pos >= jsonHeader.size() || jsonHeader[pos] == '}')
                    {
                        pos++;
                        break;
                    }
                    if (jsonHeader[pos] == ',') { pos++; continue; }

                    std::string mKey = parseStringToken(jsonHeader, pos);
                    skipWhitespace(jsonHeader, pos);
                    if (pos < jsonHeader.size() && jsonHeader[pos] == ':') pos++;
                    std::string mVal = parseStringToken(jsonHeader, pos);
                    if (!mKey.empty())
                    {
                        out.attributes[mKey] = mVal;
                    }
                }
            }
            continue;
        }

        // Tensor entry: {"dtype": "F32", "shape": [1024], "data_offsets": [0, 4096]}
        if (jsonHeader[pos] == '{')
        {
            pos++;
            TensorInfo tInfo;
            tInfo.name = key;
            int64_t offsetStart = 0;
            int64_t offsetEnd = 0;

            while (pos < jsonHeader.size())
            {
                skipWhitespace(jsonHeader, pos);
                if (pos >= jsonHeader.size() || jsonHeader[pos] == '}')
                {
                    pos++;
                    break;
                }
                if (jsonHeader[pos] == ',') { pos++; continue; }

                std::string fieldName = parseStringToken(jsonHeader, pos);
                skipWhitespace(jsonHeader, pos);
                if (pos < jsonHeader.size() && jsonHeader[pos] == ':') pos++;

                skipWhitespace(jsonHeader, pos);
                if (fieldName == "dtype")
                {
                    tInfo.dtype = parseStringToken(jsonHeader, pos);
                }
                else if (fieldName == "shape")
                {
                    tInfo.shape = parseIntArray(jsonHeader, pos);
                }
                else if (fieldName == "data_offsets")
                {
                    auto offsets = parseIntArray(jsonHeader, pos);
                    if (offsets.size() >= 2)
                    {
                        offsetStart = offsets[0];
                        offsetEnd = offsets[1];
                    }
                }
                else
                {
                    // Skip unknown field value
                    if (jsonHeader[pos] == '"') parseStringToken(jsonHeader, pos);
                    else if (jsonHeader[pos] == '[') parseIntArray(jsonHeader, pos);
                    else
                    {
                        while (pos < jsonHeader.size() && jsonHeader[pos] != ',' && jsonHeader[pos] != '}') pos++;
                    }
                }
            }

            // Calculate element count
            size_t elements = (tInfo.shape.empty()) ? 0 : 1;
            for (int64_t dim : tInfo.shape)
            {
                if (dim > 0) elements *= static_cast<size_t>(dim);
            }
            tInfo.numElements = elements;

            size_t bytesPerElem = 4;
            if (tInfo.dtype == "F16" || tInfo.dtype == "BF16" || tInfo.dtype == "I16") bytesPerElem = 2;
            else if (tInfo.dtype == "I8" || tInfo.dtype == "U8" || tInfo.dtype == "BOOL") bytesPerElem = 1;
            else if (tInfo.dtype == "F64" || tInfo.dtype == "I64") bytesPerElem = 8;

            if (offsetEnd > offsetStart)
            {
                tInfo.dataOffset = payloadBase + offsetStart;
                tInfo.byteLength = static_cast<size_t>(offsetEnd - offsetStart);
            }
            else if (!tInfo.dtype.empty() && elements > 0)
            {
                tInfo.dataOffset = payloadBase;
                tInfo.byteLength = elements * bytesPerElem;
            }

            if (!tInfo.dtype.empty() && tInfo.byteLength > 0)
            {
                out.totalParameters += elements;

                // Hash the tensor's raw binary payload
                if (tInfo.dataOffset + tInfo.byteLength <= fileSize)
                {
                    file.seekg(tInfo.dataOffset, std::ios::beg);
                    std::string tensorBytes(tInfo.byteLength, '\0');
                    file.read(&tensorBytes[0], tInfo.byteLength);
                    tInfo.tensorHash = Core::calcSHA256(tensorBytes);
                }

                out.tensors.push_back(tInfo);
            }
            else
            {
                out.attributes[key] = "(metadata object)";
            }
        }
        else
        {
            // Simple attribute at root
            if (jsonHeader[pos] == '"')
            {
                out.attributes[key] = parseStringToken(jsonHeader, pos);
            }
            else
            {
                size_t start = pos;
                while (pos < jsonHeader.size() && jsonHeader[pos] != ',' && jsonHeader[pos] != '}') pos++;
                out.attributes[key] = jsonHeader.substr(start, pos - start);
            }
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
// Format-Specific Harvesting: GGUF
// ----------------------------------------------------------------------------

bool MetadataHarvester::harvestGGUFMetadata(
    const std::filesystem::path& path,
    ModelMetadata& out
)
{
    if (!std::filesystem::exists(path)) return false;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(0, std::ios::end);
    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    if (fileSize < 24) return false;

    char magic[4];
    file.read(magic, 4);
    if (std::memcmp(magic, "GGUF", 4) != 0) return false;

    uint32_t version = 0;
    uint64_t tensorCount = 0;
    uint64_t kvCount = 0;

    file.read(reinterpret_cast<char*>(&version), 4);
    file.read(reinterpret_cast<char*>(&tensorCount), 8);
    file.read(reinterpret_cast<char*>(&kvCount), 8);

    out.format = FileFormat::GGUF;
    out.formatName = "GGUF";
    out.filePath = path;
    out.fileSize = fileSize;
    out.totalParameters = 0;
    out.attributes.clear();
    out.tensors.clear();

    out.attributes["gguf.version"] = std::to_string(version);
    out.attributes["gguf.tensor_count"] = std::to_string(tensorCount);
    out.attributes["gguf.kv_count"] = std::to_string(kvCount);

    return true;
}

// ----------------------------------------------------------------------------
// Format-Specific Harvesting: Parquet
// ----------------------------------------------------------------------------

bool MetadataHarvester::harvestParquetMetadata(
    const std::filesystem::path& path,
    DatasetMetadata& out
)
{
    if (!std::filesystem::exists(path)) return false;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(0, std::ios::end);
    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    if (fileSize < 8) return false;

    char headMagic[4], footMagic[4];
    file.read(headMagic, 4);
    file.seekg(fileSize - 4, std::ios::beg);
    file.read(footMagic, 4);

    if (std::memcmp(headMagic, "PAR1", 4) != 0 || std::memcmp(footMagic, "PAR1", 4) != 0)
    {
        return false;
    }

    out.format = FileFormat::Parquet;
    out.filePath = path;
    out.fileSize = fileSize;
    out.totalRows = 0;
    out.rowGroups = 1;
    out.columns.clear();
    out.keyValues.clear();

    out.keyValues["format"] = "Apache Parquet";
    out.keyValues["footer_magic"] = "PAR1";

    return true;
}

// ----------------------------------------------------------------------------
// Format-Specific Harvesting: JSON Configuration Files
// ----------------------------------------------------------------------------

bool MetadataHarvester::harvestJsonConfigMetadata(
    const std::filesystem::path& path,
    std::map<std::string, std::string>& out
)
{
    if (!std::filesystem::exists(path)) return false;

    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    size_t pos = 0;
    skipWhitespace(content, pos);
    if (pos >= content.size() || content[pos] != '{') return false;
    pos++;

    while (pos < content.size())
    {
        skipWhitespace(content, pos);
        if (pos >= content.size() || content[pos] == '}') break;
        if (content[pos] == ',') { pos++; continue; }

        std::string key = parseStringToken(content, pos);
        skipWhitespace(content, pos);
        if (pos < content.size() && content[pos] == ':') pos++;
        skipWhitespace(content, pos);

        if (pos < content.size() && content[pos] == '"')
        {
            out[key] = parseStringToken(content, pos);
        }
        else
        {
            size_t start = pos;
            while (pos < content.size() && content[pos] != ',' && content[pos] != '}') pos++;
            out[key] = content.substr(start, pos - start);
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
// SQLite Indexing & Persistence
// ----------------------------------------------------------------------------

bool MetadataHarvester::persistModelToSQLite(
    sqlite3* db,
    const std::string& modelId,
    const ModelMetadata& meta
)
{
    if (db == nullptr) return false;

    const char* schemaSql = R"(
        CREATE TABLE IF NOT EXISTS harvested_models (
            model_id TEXT PRIMARY KEY,
            format TEXT NOT NULL,
            file_path TEXT NOT NULL,
            file_size INTEGER NOT NULL,
            total_parameters INTEGER NOT NULL,
            harvested_at TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS model_tensors (
            model_id TEXT,
            tensor_name TEXT,
            dtype TEXT,
            shape TEXT,
            num_elements INTEGER,
            data_offset INTEGER,
            byte_length INTEGER,
            tensor_hash TEXT,
            PRIMARY KEY (model_id, tensor_name)
        );

        CREATE TABLE IF NOT EXISTS model_attributes (
            model_id TEXT,
            attr_key TEXT,
            attr_value TEXT,
            PRIMARY KEY (model_id, attr_key)
        );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, schemaSql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    // 1. Insert Model Record
    const char* insertModel = "INSERT OR REPLACE INTO harvested_models VALUES (?, ?, ?, ?, ?, datetime('now'));";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, insertModel, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, modelId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, meta.formatName.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, meta.filePath.string().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(meta.fileSize));
        sqlite3_bind_int64(stmt, 5, static_cast<sqlite3_int64>(meta.totalParameters));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // 2. Insert Tensors
    const char* insertTensor = "INSERT OR REPLACE INTO model_tensors VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, insertTensor, -1, &stmt, nullptr) == SQLITE_OK)
    {
        for (const auto& t : meta.tensors)
        {
            sqlite3_reset(stmt);
            std::string shapeStr = shapeToString(t.shape);
            sqlite3_bind_text(stmt, 1, modelId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, t.name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, t.dtype.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, shapeStr.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int64(stmt, 5, static_cast<sqlite3_int64>(t.numElements));
            sqlite3_bind_int64(stmt, 6, static_cast<sqlite3_int64>(t.dataOffset));
            sqlite3_bind_int64(stmt, 7, static_cast<sqlite3_int64>(t.byteLength));
            sqlite3_bind_text(stmt, 8, t.tensorHash.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }

    // 3. Insert Attributes
    const char* insertAttr = "INSERT OR REPLACE INTO model_attributes VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(db, insertAttr, -1, &stmt, nullptr) == SQLITE_OK)
    {
        for (const auto& kv : meta.attributes)
        {
            sqlite3_reset(stmt);
            sqlite3_bind_text(stmt, 1, modelId.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, kv.first.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, kv.second.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }

    return true;
}

bool MetadataHarvester::persistDatasetToSQLite(
    sqlite3* db,
    const std::string& datasetId,
    const DatasetMetadata& meta
)
{
    if (db == nullptr) return false;

    const char* schemaSql = R"(
        CREATE TABLE IF NOT EXISTS harvested_datasets (
            dataset_id TEXT PRIMARY KEY,
            format TEXT NOT NULL,
            file_path TEXT NOT NULL,
            file_size INTEGER NOT NULL,
            total_rows INTEGER NOT NULL,
            row_groups INTEGER NOT NULL,
            harvested_at TEXT NOT NULL
        );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, schemaSql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }

    const char* insertDs = "INSERT OR REPLACE INTO harvested_datasets VALUES (?, 'Parquet', ?, ?, ?, ?, datetime('now'));";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, insertDs, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, datasetId.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, meta.filePath.string().c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(meta.fileSize));
        sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(meta.totalRows));
        sqlite3_bind_int64(stmt, 5, static_cast<sqlite3_int64>(meta.rowGroups));
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    return true;
}

} // namespace Core
