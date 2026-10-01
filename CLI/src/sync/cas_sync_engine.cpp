#include "cas_sync_engine.hpp"
#ifndef CURL_STATICLIB
#define CURL_STATICLIB
#endif
#include <curl/curl.h>
#include "../hashing/hashing.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <set>

namespace fs = std::filesystem;

namespace Core::Sync {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::vector<uint8_t> readBinaryFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    std::streamsize size = file.tellg();
    if (size <= 0) return {};
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) return {};
    return buffer;
}

static bool writeBinaryFile(const std::string& filePath, const std::vector<uint8_t>& buffer) {
    fs::path p(filePath);
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path());
    }
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    return file.good();
}

static std::string calculateSha256(const std::vector<uint8_t>& buf) {
    return Core::calcSHA256(std::string(reinterpret_cast<const char*>(buf.data()), buf.size()));
}

static std::string calculateSha256(const uint8_t* data, size_t size) {
    return Core::calcSHA256(std::string(reinterpret_cast<const char*>(data), size));
}

static std::string escapeJsonString(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        if (c == '"') ss << "\\\"";
        else if (c == '\\') ss << "\\\\";
        else if (c == '\b') ss << "\\b";
        else if (c == '\f') ss << "\\f";
        else if (c == '\n') ss << "\\n";
        else if (c == '\r') ss << "\\r";
        else if (c == '\t') ss << "\\t";
        else ss << c;
    }
    return ss.str();
}

static std::string extractJsonField(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
        pos++;
    }
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        size_t endQuote = json.find('"', pos + 1);
        if (endQuote == std::string::npos) return "";
        return json.substr(pos + 1, endQuote - pos - 1);
    } else {
        size_t endVal = json.find_first_of(",}\r\n \t", pos);
        if (endVal == std::string::npos) endVal = json.size();
        return json.substr(pos, endVal - pos);
    }
}

// ---------------------------------------------------------------------------
// FileManifest Serialization & Deserialization
// ---------------------------------------------------------------------------
std::string FileManifest::serializeJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"file_path\": \"" << escapeJsonString(file_path) << "\",\n";
    ss << "  \"file_format\": \"" << escapeJsonString(file_format) << "\",\n";
    ss << "  \"file_category\": \"" << escapeJsonString(file_category) << "\",\n";
    ss << "  \"file_sha256\": \"" << file_sha256 << "\",\n";
    ss << "  \"total_raw_size\": " << total_raw_size << ",\n";
    ss << "  \"total_compressed_size\": " << total_compressed_size << ",\n";
    ss << "  \"created_at\": " << created_at << ",\n";
    ss << "  \"chunks\": [\n";
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& c = chunks[i];
        ss << "    {\n";
        ss << "      \"chunk_index\": " << c.chunk_index << ",\n";
        ss << "      \"chunk_type\": \"" << escapeJsonString(c.chunk_type) << "\",\n";
        ss << "      \"raw_hash\": \"" << c.raw_hash << "\",\n";
        ss << "      \"raw_size\": " << c.raw_size << ",\n";
        ss << "      \"compressed_size\": " << c.compressed_size << ",\n";
        ss << "      \"codec_method\": \"" << escapeJsonString(c.codec_method) << "\",\n";
        ss << "      \"offset\": " << c.offset << "\n";
        ss << "    }" << (i + 1 < chunks.size() ? "," : "") << "\n";
    }
    ss << "  ]\n";
    ss << "}";
    return ss.str();
}

FileManifest FileManifest::deserializeJson(const std::string& jsonStr) {
    FileManifest manifest;
    manifest.file_path = extractJsonField(jsonStr, "file_path");
    manifest.file_format = extractJsonField(jsonStr, "file_format");
    manifest.file_category = extractJsonField(jsonStr, "file_category");
    manifest.file_sha256 = extractJsonField(jsonStr, "file_sha256");

    std::string rawSz = extractJsonField(jsonStr, "total_raw_size");
    if (!rawSz.empty()) manifest.total_raw_size = std::stoull(rawSz);

    std::string compSz = extractJsonField(jsonStr, "total_compressed_size");
    if (!compSz.empty()) manifest.total_compressed_size = std::stoull(compSz);

    std::string crAt = extractJsonField(jsonStr, "created_at");
    if (!crAt.empty()) manifest.created_at = std::stoull(crAt);

    size_t chunksPos = jsonStr.find("\"chunks\"");
    if (chunksPos != std::string::npos) {
        size_t arrayStart = jsonStr.find('[', chunksPos);
        size_t arrayEnd = jsonStr.rfind(']');
        if (arrayStart != std::string::npos && arrayEnd != std::string::npos && arrayEnd > arrayStart) {
            std::string chunksArray = jsonStr.substr(arrayStart, arrayEnd - arrayStart + 1);
            size_t pos = 0;
            while ((pos = chunksArray.find('{', pos)) != std::string::npos) {
                size_t objEnd = chunksArray.find('}', pos);
                if (objEnd == std::string::npos) break;
                std::string objStr = chunksArray.substr(pos, objEnd - pos + 1);

                ChunkSyncRecord rec;
                std::string idxStr = extractJsonField(objStr, "chunk_index");
                if (!idxStr.empty()) rec.chunk_index = std::stoull(idxStr);
                rec.chunk_type = extractJsonField(objStr, "chunk_type");
                rec.raw_hash = extractJsonField(objStr, "raw_hash");
                std::string rSize = extractJsonField(objStr, "raw_size");
                if (!rSize.empty()) rec.raw_size = std::stoull(rSize);
                std::string cSize = extractJsonField(objStr, "compressed_size");
                if (!cSize.empty()) rec.compressed_size = std::stoull(cSize);
                rec.codec_method = extractJsonField(objStr, "codec_method");
                std::string offStr = extractJsonField(objStr, "offset");
                if (!offStr.empty()) rec.offset = std::stoull(offStr);

                manifest.chunks.push_back(rec);
                pos = objEnd + 1;
            }
        }
    }

    return manifest;
}

std::string SyncResult::summaryJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"file_path\": \"" << escapeJsonString(file_path) << "\",\n";
    ss << "  \"file_sha256\": \"" << file_sha256 << "\",\n";
    ss << "  \"file_format\": \"" << escapeJsonString(file_format) << "\",\n";
    ss << "  \"manifest_id\": \"" << manifest_id << "\",\n";
    ss << "  \"total_chunks\": " << total_chunks << ",\n";
    ss << "  \"uploaded_chunks\": " << uploaded_chunks << ",\n";
    ss << "  \"skipped_chunks\": " << skipped_chunks << ",\n";
    ss << "  \"raw_bytes_total\": " << raw_bytes_total << ",\n";
    ss << "  \"bytes_uploaded\": " << bytes_uploaded << ",\n";
    ss << "  \"bytes_saved_by_dedup\": " << bytes_saved_by_dedup << ",\n";
    ss << "  \"deduplication_ratio\": " << std::fixed << std::setprecision(2) << deduplication_ratio << ",\n";
    ss << "  \"success\": " << (success ? "true" : "false");
    if (!error_message.empty()) {
        ss << ",\n  \"error\": \"" << escapeJsonString(error_message) << "\"";
    }
    ss << "\n}";
    return ss.str();
}

// ---------------------------------------------------------------------------
// DirectoryCASRemote Implementation
// ---------------------------------------------------------------------------
DirectoryCASRemote::DirectoryCASRemote(const std::string& rootDirectory)
    : rootDir_(rootDirectory) {
    fs::create_directories(fs::path(rootDir_) / "chunks");
    fs::create_directories(fs::path(rootDir_) / "manifests");
}

std::string DirectoryCASRemote::getChunkPath(const std::string& raw_hash) const {
    std::string prefix = raw_hash.size() >= 2 ? raw_hash.substr(0, 2) : "00";
    return (fs::path(rootDir_) / "chunks" / prefix / (raw_hash + ".cas")).string();
}

std::string DirectoryCASRemote::getManifestPath(const std::string& manifest_id) const {
    std::string prefix = manifest_id.size() >= 2 ? manifest_id.substr(0, 2) : "00";
    return (fs::path(rootDir_) / "manifests" / prefix / (manifest_id + ".json")).string();
}

bool DirectoryCASRemote::hasChunk(const std::string& raw_hash) {
    return fs::exists(getChunkPath(raw_hash));
}

std::vector<std::string> DirectoryCASRemote::queryMissingChunks(const std::vector<std::string>& raw_hashes) {
    std::vector<std::string> missing;
    for (const auto& h : raw_hashes) {
        if (!hasChunk(h)) {
            missing.push_back(h);
        }
    }
    return missing;
}

bool DirectoryCASRemote::putChunk(const std::string& raw_hash, const std::vector<uint8_t>& payload) {
    std::string p = getChunkPath(raw_hash);
    return writeBinaryFile(p, payload);
}

bool DirectoryCASRemote::getChunk(const std::string& raw_hash, std::vector<uint8_t>& out_payload) {
    std::string p = getChunkPath(raw_hash);
    out_payload = readBinaryFile(p);
    return !out_payload.empty();
}

bool DirectoryCASRemote::putManifest(const std::string& manifest_id, const std::string& manifest_json) {
    std::string p = getManifestPath(manifest_id);
    fs::create_directories(fs::path(p).parent_path());
    std::ofstream out(p);
    if (!out.is_open()) return false;
    out << manifest_json;
    return out.good();
}

bool DirectoryCASRemote::getManifest(const std::string& manifest_id, std::string& out_manifest_json) {
    std::string p = getManifestPath(manifest_id);
    std::ifstream in(p);
    if (!in.is_open()) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out_manifest_json = ss.str();
    return true;
}

// ---------------------------------------------------------------------------
// HttpMinioCASRemote Implementation (libcurl)
// ---------------------------------------------------------------------------
static size_t curlStringWriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t total = size * nmemb;
    userp->append(static_cast<char*>(contents), total);
    return total;
}

static size_t curlBinaryWriteCallback(void* contents, size_t size, size_t nmemb, std::vector<uint8_t>* userp) {
    size_t total = size * nmemb;
    const uint8_t* bytePtr = static_cast<const uint8_t*>(contents);
    userp->insert(userp->end(), bytePtr, bytePtr + total);
    return total;
}

HttpMinioCASRemote::HttpMinioCASRemote(const std::string& endpointUrl,
                                       const std::string& bucketName,
                                       const std::string& accessKey,
                                       const std::string& secretKey)
    : endpointUrl_(endpointUrl), bucketName_(bucketName), accessKey_(accessKey), secretKey_(secretKey) {
    if (!endpointUrl_.empty() && endpointUrl_.back() == '/') {
        endpointUrl_.pop_back();
    }
}

std::string HttpMinioCASRemote::makeUrl(const std::string& subpath) const {
    return endpointUrl_ + "/" + bucketName_ + "/" + subpath;
}

bool HttpMinioCASRemote::hasChunk(const std::string& raw_hash) {
    std::string prefix = raw_hash.size() >= 2 ? raw_hash.substr(0, 2) : "00";
    std::string url = makeUrl("chunks/" + prefix + "/" + raw_hash + ".cas");

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD request
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && responseCode == 200);
}

std::vector<std::string> HttpMinioCASRemote::queryMissingChunks(const std::vector<std::string>& raw_hashes) {
    std::vector<std::string> missing;
    for (const auto& h : raw_hashes) {
        if (!hasChunk(h)) {
            missing.push_back(h);
        }
    }
    return missing;
}

bool HttpMinioCASRemote::putChunk(const std::string& raw_hash, const std::vector<uint8_t>& payload) {
    std::string prefix = raw_hash.size() >= 2 ? raw_hash.substr(0, 2) : "00";
    std::string url = makeUrl("chunks/" + prefix + "/" + raw_hash + ".cas");

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && responseCode >= 200 && responseCode < 300);
}

bool HttpMinioCASRemote::getChunk(const std::string& raw_hash, std::vector<uint8_t>& out_payload) {
    std::string prefix = raw_hash.size() >= 2 ? raw_hash.substr(0, 2) : "00";
    std::string url = makeUrl("chunks/" + prefix + "/" + raw_hash + ".cas");

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    out_payload.clear();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlBinaryWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_payload);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && responseCode == 200);
}

bool HttpMinioCASRemote::putManifest(const std::string& manifest_id, const std::string& manifest_json) {
    std::string prefix = manifest_id.size() >= 2 ? manifest_id.substr(0, 2) : "00";
    std::string url = makeUrl("manifests/" + prefix + "/" + manifest_id + ".json");

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, manifest_json.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(manifest_json.size()));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && responseCode >= 200 && responseCode < 300);
}

bool HttpMinioCASRemote::getManifest(const std::string& manifest_id, std::string& out_manifest_json) {
    std::string prefix = manifest_id.size() >= 2 ? manifest_id.substr(0, 2) : "00";
    std::string url = makeUrl("manifests/" + prefix + "/" + manifest_id + ".json");

    CURL* curl = curl_easy_init();
    if (!curl) return false;

    out_manifest_json.clear();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlStringWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_manifest_json);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && responseCode == 200);
}

std::unique_ptr<ICASRemoteStorage> createCASRemote(const std::string& endpointOrPath,
                                                  const std::string& bucketName) {
    if (endpointOrPath.rfind("http://", 0) == 0 || endpointOrPath.rfind("https://", 0) == 0) {
        return std::make_unique<HttpMinioCASRemote>(endpointOrPath, bucketName);
    }
    std::string cleanPath = endpointOrPath;
    if (cleanPath.rfind("cas://", 0) == 0) {
        cleanPath = cleanPath.substr(6);
    }
    return std::make_unique<DirectoryCASRemote>(cleanPath);
}

// ---------------------------------------------------------------------------
// CASSyncEngine CAS Primitives
// ---------------------------------------------------------------------------
std::vector<std::string> CASSyncEngine::queryRemoteMissingChunks(ICASRemoteStorage& remote,
                                                               const std::vector<std::string>& rawHashes) {
    return remote.queryMissingChunks(rawHashes);
}

bool CASSyncEngine::uploadChunkToRemoteCAS(ICASRemoteStorage& remote,
                                         const std::string& rawHash,
                                         const std::vector<uint8_t>& containerPayload) {
    return remote.putChunk(rawHash, containerPayload);
}

bool CASSyncEngine::downloadChunkFromRemoteCAS(ICASRemoteStorage& remote,
                                             const std::string& rawHash,
                                             std::vector<uint8_t>& outRawData) {
    std::vector<uint8_t> containerPayload;
    if (!remote.getChunk(rawHash, containerPayload) || containerPayload.empty()) {
        return false;
    }

    std::string decodedRawHash;
    std::string decodedMethod;
    outRawData = Core::PolymorphicCodec::decompressContainer(
        containerPayload.data(),
        containerPayload.size(),
        &decodedRawHash,
        &decodedMethod
    );

    if (outRawData.empty() && containerPayload.size() > 80) {
        return false;
    }

    // Verify bit-exact hash
    return (decodedRawHash == rawHash);
}

FileManifest CASSyncEngine::assembleRemoteManifest(const std::string& filePath,
                                                 const std::vector<ChunkSyncRecord>& records,
                                                 const std::string& formatName,
                                                 const std::string& categoryName,
                                                 const std::string& fileHash,
                                                 size_t totalRawSize) {
    FileManifest manifest;
    manifest.file_path = filePath;
    manifest.file_format = formatName;
    manifest.file_category = categoryName;
    manifest.file_sha256 = fileHash;
    manifest.total_raw_size = totalRawSize;
    manifest.chunks = records;

    size_t totalComp = 0;
    for (const auto& c : records) {
        totalComp += c.compressed_size;
    }
    manifest.total_compressed_size = totalComp;

    auto now = std::chrono::system_clock::now();
    manifest.created_at = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    return manifest;
}

// ---------------------------------------------------------------------------
// Format-Specific Sync: SafeTensors Models
// ---------------------------------------------------------------------------
SyncResult CASSyncEngine::syncSafeTensorsModelWithRemoteCAS(const std::string& filePath,
                                                          ICASRemoteStorage& remote) {
    SyncResult result;
    result.file_path = filePath;
    result.file_format = "SafeTensors";

    std::vector<uint8_t> fileBytes = readBinaryFile(filePath);
    if (fileBytes.size() < 10) {
        result.error_message = "File is empty or cannot be opened";
        return result;
    }

    result.raw_bytes_total = fileBytes.size();
    result.file_sha256 = calculateSha256(fileBytes);

    auto inspection = Core::FileInspector::inspect(filePath);

    // Asymmetric & boundary-aligned chunking:
    // Header slice (volatile metadata JSON) vs Tensor weights (static payload)
    auto chunks = Core::AsymmetricChunker::chunkSafeTensorsWithBoundarySplit(fileBytes.data(), fileBytes.size(), inspection);
    result.total_chunks = chunks.size();

    std::vector<std::string> rawHashes;
    for (const auto& c : chunks) {
        rawHashes.push_back(c.sha256);
    }

    // Remote delta negotiation
    std::vector<std::string> missing = queryRemoteMissingChunks(remote, rawHashes);
    std::set<std::string> missingSet(missing.begin(), missing.end());

    std::vector<ChunkSyncRecord> manifestRecords;
    manifestRecords.reserve(chunks.size());

    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& c = chunks[i];
        const uint8_t* cPtr = fileBytes.data() + c.offset;

        Core::CodecResult codecRes;
        if (c.category == Core::ChunkCategory::MetadataHeader) {
            codecRes = Core::PolymorphicCodec::compressMetadataWithZlib(cPtr, c.length);
        } else {
            // Polymorphic byte-shuffle for tensor weights
            codecRes = Core::PolymorphicCodec::compressTensorWithByteShuffle(cPtr, c.length, 4);
        }

        ChunkSyncRecord rec;
        rec.chunk_index = i;
        rec.raw_hash = codecRes.rawSha256;
        rec.raw_size = codecRes.rawSize;
        rec.compressed_size = codecRes.compressedSize;
        rec.codec_method = codecRes.codecMethod;
        rec.offset = c.offset;
        rec.chunk_type = (c.category == Core::ChunkCategory::MetadataHeader) ? "HEADER_JSON" : "TENSOR_DATA";

        if (missingSet.find(codecRes.rawSha256) == missingSet.end()) {
            // Already present on remote CAS -> 0 bytes sent over network
            rec.status = ChunkSyncStatus::ALREADY_EXISTS_REMOTE;
            result.skipped_chunks++;
            result.bytes_saved_by_dedup += rec.raw_size;
        } else {
            // Missing on remote CAS -> upload compressed container
            bool ok = uploadChunkToRemoteCAS(remote, codecRes.rawSha256, codecRes.payload);
            if (!ok) {
                rec.status = ChunkSyncStatus::FAILED;
                result.error_message = "Failed to upload chunk " + codecRes.rawSha256;
                result.success = false;
                return result;
            }
            rec.status = ChunkSyncStatus::UPLOADED;
            result.uploaded_chunks++;
            result.bytes_uploaded += rec.compressed_size;
        }

        manifestRecords.push_back(rec);
    }

    // Assemble and upload remote file manifest
    FileManifest manifest = assembleRemoteManifest(filePath, manifestRecords, "SafeTensors", "ModelWeights", result.file_sha256, result.raw_bytes_total);
    result.manifest_id = result.file_sha256;
    result.manifest_json = manifest.serializeJson();

    if (!remote.putManifest(result.manifest_id, result.manifest_json)) {
        result.error_message = "Failed to upload manifest to remote CAS";
        return result;
    }

    if (result.raw_bytes_total > 0) {
        result.deduplication_ratio = (static_cast<double>(result.bytes_saved_by_dedup) / static_cast<double>(result.raw_bytes_total)) * 100.0;
    }

    result.success = true;
    return result;
}

// ---------------------------------------------------------------------------
// Format-Specific Sync: Parquet Datasets
// ---------------------------------------------------------------------------
SyncResult CASSyncEngine::syncParquetDatasetWithRemoteCAS(const std::string& filePath,
                                                         ICASRemoteStorage& remote) {
    SyncResult result;
    result.file_path = filePath;
    result.file_format = "Parquet";

    std::vector<uint8_t> fileBytes = readBinaryFile(filePath);
    if (fileBytes.size() < 12) {
        result.error_message = "Parquet file too small or cannot be opened";
        return result;
    }

    result.raw_bytes_total = fileBytes.size();
    result.file_sha256 = calculateSha256(fileBytes);

    auto inspection = Core::FileInspector::inspect(filePath);
    auto chunks = Core::AsymmetricChunker::chunkParquetWithStructuralSlices(fileBytes.data(), fileBytes.size(), inspection);
    result.total_chunks = chunks.size();

    std::vector<std::string> rawHashes;
    for (const auto& c : chunks) rawHashes.push_back(c.sha256);

    std::vector<std::string> missing = queryRemoteMissingChunks(remote, rawHashes);
    std::set<std::string> missingSet(missing.begin(), missing.end());

    std::vector<ChunkSyncRecord> manifestRecords;
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& c = chunks[i];
        const uint8_t* cPtr = fileBytes.data() + c.offset;

        auto codecRes = Core::PolymorphicCodec::compressChunk(cPtr, c.length, c.category, Core::FileFormat::Parquet);

        ChunkSyncRecord rec;
        rec.chunk_index = i;
        rec.raw_hash = codecRes.rawSha256;
        rec.raw_size = codecRes.rawSize;
        rec.compressed_size = codecRes.compressedSize;
        rec.codec_method = codecRes.codecMethod;
        rec.offset = c.offset;
        rec.chunk_type = (c.category == Core::ChunkCategory::MetadataHeader) ? "PARQUET_METADATA" : "PARQUET_ROWGROUP";

        if (missingSet.find(codecRes.rawSha256) == missingSet.end()) {
            rec.status = ChunkSyncStatus::ALREADY_EXISTS_REMOTE;
            result.skipped_chunks++;
            result.bytes_saved_by_dedup += rec.raw_size;
        } else {
            bool ok = uploadChunkToRemoteCAS(remote, codecRes.rawSha256, codecRes.payload);
            if (!ok) {
                result.error_message = "Failed to upload parquet chunk " + codecRes.rawSha256;
                return result;
            }
            rec.status = ChunkSyncStatus::UPLOADED;
            result.uploaded_chunks++;
            result.bytes_uploaded += rec.compressed_size;
        }
        manifestRecords.push_back(rec);
    }

    FileManifest manifest = assembleRemoteManifest(filePath, manifestRecords, "Parquet", "Dataset", result.file_sha256, result.raw_bytes_total);
    result.manifest_id = result.file_sha256;
    result.manifest_json = manifest.serializeJson();
    remote.putManifest(result.manifest_id, result.manifest_json);

    if (result.raw_bytes_total > 0) {
        result.deduplication_ratio = (static_cast<double>(result.bytes_saved_by_dedup) / static_cast<double>(result.raw_bytes_total)) * 100.0;
    }
    result.success = true;
    return result;
}

// ---------------------------------------------------------------------------
// Format-Specific Sync: Pre-compressed Media & PDFs (Bypass Codec)
// ---------------------------------------------------------------------------
SyncResult CASSyncEngine::syncMediaBlobWithRemoteCAS(const std::string& filePath,
                                                    ICASRemoteStorage& remote) {
    SyncResult result;
    result.file_path = filePath;
    result.file_format = "Media";

    std::vector<uint8_t> fileBytes = readBinaryFile(filePath);
    if (fileBytes.empty()) {
        result.error_message = "Media file is empty or cannot be opened";
        return result;
    }

    result.raw_bytes_total = fileBytes.size();
    result.file_sha256 = calculateSha256(fileBytes);

    auto inspection = Core::FileInspector::inspect(filePath);
    auto chunks = Core::AsymmetricChunker::chunkFile(filePath, inspection);
    result.total_chunks = chunks.size();

    std::vector<std::string> rawHashes;
    for (const auto& c : chunks) rawHashes.push_back(c.sha256);

    std::vector<std::string> missing = queryRemoteMissingChunks(remote, rawHashes);
    std::set<std::string> missingSet(missing.begin(), missing.end());

    std::vector<ChunkSyncRecord> manifestRecords;
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& c = chunks[i];
        const uint8_t* cPtr = fileBytes.data() + c.offset;

        // Zero-CPU bypass codec for pre-compressed media
        auto codecRes = Core::PolymorphicCodec::compressMediaWithBypass(cPtr, c.length);

        ChunkSyncRecord rec;
        rec.chunk_index = i;
        rec.raw_hash = codecRes.rawSha256;
        rec.raw_size = codecRes.rawSize;
        rec.compressed_size = codecRes.compressedSize;
        rec.codec_method = codecRes.codecMethod;
        rec.offset = c.offset;
        rec.chunk_type = "MEDIA_BLOB";

        if (missingSet.find(codecRes.rawSha256) == missingSet.end()) {
            rec.status = ChunkSyncStatus::ALREADY_EXISTS_REMOTE;
            result.skipped_chunks++;
            result.bytes_saved_by_dedup += rec.raw_size;
        } else {
            bool ok = uploadChunkToRemoteCAS(remote, codecRes.rawSha256, codecRes.payload);
            if (!ok) {
                result.error_message = "Failed to upload media chunk " + codecRes.rawSha256;
                return result;
            }
            rec.status = ChunkSyncStatus::UPLOADED;
            result.uploaded_chunks++;
            result.bytes_uploaded += rec.compressed_size;
        }
        manifestRecords.push_back(rec);
    }

    FileManifest manifest = assembleRemoteManifest(filePath, manifestRecords, "Media", "MediaArtifact", result.file_sha256, result.raw_bytes_total);
    result.manifest_id = result.file_sha256;
    result.manifest_json = manifest.serializeJson();
    remote.putManifest(result.manifest_id, result.manifest_json);

    if (result.raw_bytes_total > 0) {
        result.deduplication_ratio = (static_cast<double>(result.bytes_saved_by_dedup) / static_cast<double>(result.raw_bytes_total)) * 100.0;
    }
    result.success = true;
    return result;
}

// ---------------------------------------------------------------------------
// Format-Specific Sync: Generic Code / Binary / Config Files
// ---------------------------------------------------------------------------
SyncResult CASSyncEngine::syncGenericFileWithRemoteCAS(const std::string& filePath,
                                                      ICASRemoteStorage& remote) {
    SyncResult result;
    result.file_path = filePath;
    result.file_format = "Generic";

    std::vector<uint8_t> fileBytes = readBinaryFile(filePath);
    if (fileBytes.empty()) {
        result.error_message = "File is empty or cannot be opened";
        return result;
    }

    result.raw_bytes_total = fileBytes.size();
    result.file_sha256 = calculateSha256(fileBytes);

    auto inspection = Core::FileInspector::inspect(filePath);
    auto chunks = Core::AsymmetricChunker::chunkFile(filePath, inspection);
    result.total_chunks = chunks.size();

    std::vector<std::string> rawHashes;
    for (const auto& c : chunks) rawHashes.push_back(c.sha256);

    std::vector<std::string> missing = queryRemoteMissingChunks(remote, rawHashes);
    std::set<std::string> missingSet(missing.begin(), missing.end());

    std::vector<ChunkSyncRecord> manifestRecords;
    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& c = chunks[i];
        const uint8_t* cPtr = fileBytes.data() + c.offset;

        auto codecRes = Core::PolymorphicCodec::compressChunk(cPtr, c.length, c.category, inspection.format);

        ChunkSyncRecord rec;
        rec.chunk_index = i;
        rec.raw_hash = codecRes.rawSha256;
        rec.raw_size = codecRes.rawSize;
        rec.compressed_size = codecRes.compressedSize;
        rec.codec_method = codecRes.codecMethod;
        rec.offset = c.offset;
        rec.chunk_type = "FASTCDC_CHUNK";

        if (missingSet.find(codecRes.rawSha256) == missingSet.end()) {
            rec.status = ChunkSyncStatus::ALREADY_EXISTS_REMOTE;
            result.skipped_chunks++;
            result.bytes_saved_by_dedup += rec.raw_size;
        } else {
            bool ok = uploadChunkToRemoteCAS(remote, codecRes.rawSha256, codecRes.payload);
            if (!ok) {
                result.error_message = "Failed to upload generic chunk " + codecRes.rawSha256;
                return result;
            }
            rec.status = ChunkSyncStatus::UPLOADED;
            result.uploaded_chunks++;
            result.bytes_uploaded += rec.compressed_size;
        }
        manifestRecords.push_back(rec);
    }

    FileManifest manifest = assembleRemoteManifest(filePath, manifestRecords, "Generic", "GenericCodeOrBinary", result.file_sha256, result.raw_bytes_total);
    result.manifest_id = result.file_sha256;
    result.manifest_json = manifest.serializeJson();
    remote.putManifest(result.manifest_id, result.manifest_json);

    if (result.raw_bytes_total > 0) {
        result.deduplication_ratio = (static_cast<double>(result.bytes_saved_by_dedup) / static_cast<double>(result.raw_bytes_total)) * 100.0;
    }
    result.success = true;
    return result;
}

// ---------------------------------------------------------------------------
// Format Dispatcher
// ---------------------------------------------------------------------------
SyncResult CASSyncEngine::syncFileWithRemoteCAS(const std::string& filePath, ICASRemoteStorage& remote) {
    auto inspection = Core::FileInspector::inspect(filePath);
    if (inspection.format == Core::FileFormat::SafeTensors) {
        return syncSafeTensorsModelWithRemoteCAS(filePath, remote);
    } else if (inspection.format == Core::FileFormat::Parquet) {
        return syncParquetDatasetWithRemoteCAS(filePath, remote);
    } else if (inspection.category == Core::FileCategory::Media ||
               inspection.format == Core::FileFormat::PDF) {
        return syncMediaBlobWithRemoteCAS(filePath, remote);
    } else {
        return syncGenericFileWithRemoteCAS(filePath, remote);
    }
}

// ---------------------------------------------------------------------------
// Bit-Exact Reconstruction / Download from Remote CAS
// ---------------------------------------------------------------------------
bool CASSyncEngine::reconstructSafeTensorsFromRemoteCAS(const FileManifest& manifest,
                                                       const std::string& outputPath,
                                                       ICASRemoteStorage& remote) {
    return reconstructGenericFromRemoteCAS(manifest, outputPath, remote);
}

bool CASSyncEngine::reconstructParquetFromRemoteCAS(const FileManifest& manifest,
                                                   const std::string& outputPath,
                                                   ICASRemoteStorage& remote) {
    return reconstructGenericFromRemoteCAS(manifest, outputPath, remote);
}

bool CASSyncEngine::reconstructMediaFromRemoteCAS(const FileManifest& manifest,
                                                 const std::string& outputPath,
                                                 ICASRemoteStorage& remote) {
    return reconstructGenericFromRemoteCAS(manifest, outputPath, remote);
}

bool CASSyncEngine::reconstructGenericFromRemoteCAS(const FileManifest& manifest,
                                                   const std::string& outputPath,
                                                   ICASRemoteStorage& remote) {
    std::vector<uint8_t> fullBuffer(manifest.total_raw_size, 0);

    for (const auto& c : manifest.chunks) {
        std::vector<uint8_t> rawChunk;
        if (!downloadChunkFromRemoteCAS(remote, c.raw_hash, rawChunk)) {
            std::cerr << "[CASSyncEngine] Failed to download or verify chunk " << c.raw_hash << std::endl;
            return false;
        }

        if (c.offset + rawChunk.size() > fullBuffer.size()) {
            std::cerr << "[CASSyncEngine] Chunk offset out of bounds" << std::endl;
            return false;
        }

        std::memcpy(fullBuffer.data() + c.offset, rawChunk.data(), rawChunk.size());
    }

    // Integrity check of reassembled file against manifest SHA-256
    std::string reassembledHash = calculateSha256(fullBuffer);
    if (reassembledHash != manifest.file_sha256) {
        std::cerr << "[CASSyncEngine] SHA-256 mismatch on reconstructed file! Expected: "
                  << manifest.file_sha256 << " vs Got: " << reassembledHash << std::endl;
        return false;
    }

    return writeBinaryFile(outputPath, fullBuffer);
}

bool CASSyncEngine::reconstructFileFromRemoteCAS(const std::string& manifestId,
                                                const std::string& outputPath,
                                                ICASRemoteStorage& remote) {
    std::string manifestJson;
    if (!remote.getManifest(manifestId, manifestJson)) {
        std::cerr << "[CASSyncEngine] Manifest not found on remote CAS: " << manifestId << std::endl;
        return false;
    }

    FileManifest manifest = FileManifest::deserializeJson(manifestJson);
    if (manifest.chunks.empty() && manifest.total_raw_size > 0) {
        std::cerr << "[CASSyncEngine] Corrupted manifest JSON for ID: " << manifestId << std::endl;
        return false;
    }

    return reconstructGenericFromRemoteCAS(manifest, outputPath, remote);
}

} // namespace Core::Sync
