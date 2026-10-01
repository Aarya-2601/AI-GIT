#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <map>
#include "../inspection/file_inspector.hpp"
#include "../chunking/asymmetric_chunker.hpp"
#include "../compression/polymorphic_codec.hpp"

namespace Core::Sync {

enum class ChunkSyncStatus {
    PENDING,
    ALREADY_EXISTS_REMOTE, // 100% deduplicated, 0 bytes sent over network
    UPLOADED,              // Uploaded to CAS
    FAILED
};

struct ChunkSyncRecord {
    size_t chunk_index{0};
    std::string chunk_type;       // e.g. "MetadataHeader", "TensorPayload", "ParquetSlice", "MediaBlob", "GenericData"
    std::string raw_hash;         // SHA-256 content key of uncompressed payload
    size_t raw_size{0};
    size_t compressed_size{0};
    std::string codec_method;     // Explicit method name (e.g. "compressTensorWithByteShuffle")
    size_t offset{0};
    ChunkSyncStatus status{ChunkSyncStatus::PENDING};
};

struct FileManifest {
    std::string file_path;
    std::string file_format;
    std::string file_category;
    size_t total_raw_size{0};
    size_t total_compressed_size{0};
    std::string file_sha256;
    std::vector<ChunkSyncRecord> chunks;
    uint64_t created_at{0};

    std::string serializeJson() const;
    static FileManifest deserializeJson(const std::string& jsonStr);
};

struct SyncResult {
    std::string file_path;
    std::string file_sha256;
    std::string manifest_id;
    std::string file_format;
    size_t total_chunks{0};
    size_t uploaded_chunks{0};
    size_t skipped_chunks{0};      // Deduplicated chunks
    size_t raw_bytes_total{0};
    size_t bytes_uploaded{0};
    size_t bytes_saved_by_dedup{0};
    double deduplication_ratio{0.0}; // Percentage of bytes deduplicated
    bool success{false};
    std::string error_message;
    std::string manifest_json;

    std::string summaryJson() const;
};

// ---------------------------------------------------------------------------
// CAS Remote Storage Interface
// ---------------------------------------------------------------------------
class ICASRemoteStorage {
public:
    virtual ~ICASRemoteStorage() = default;

    virtual bool hasChunk(const std::string& raw_hash) = 0;
    virtual std::vector<std::string> queryMissingChunks(const std::vector<std::string>& raw_hashes) = 0;
    virtual bool putChunk(const std::string& raw_hash, const std::vector<uint8_t>& payload) = 0;
    virtual bool getChunk(const std::string& raw_hash, std::vector<uint8_t>& out_payload) = 0;
    
    virtual bool putManifest(const std::string& manifest_id, const std::string& manifest_json) = 0;
    virtual bool getManifest(const std::string& manifest_id, std::string& out_manifest_json) = 0;

    virtual std::string getRemoteUri() const = 0;
    virtual std::string getRemoteType() const = 0;
};

// Local filesystem directory CAS store (for zero-latency offline testing and local MinIO cache)
class DirectoryCASRemote : public ICASRemoteStorage {
public:
    explicit DirectoryCASRemote(const std::string& rootDirectory);

    bool hasChunk(const std::string& raw_hash) override;
    std::vector<std::string> queryMissingChunks(const std::vector<std::string>& raw_hashes) override;
    bool putChunk(const std::string& raw_hash, const std::vector<uint8_t>& payload) override;
    bool getChunk(const std::string& raw_hash, std::vector<uint8_t>& out_payload) override;

    bool putManifest(const std::string& manifest_id, const std::string& manifest_json) override;
    bool getManifest(const std::string& manifest_id, std::string& out_manifest_json) override;

    std::string getRemoteUri() const override { return rootDir_; }
    std::string getRemoteType() const override { return "DirectoryCAS"; }

private:
    std::string rootDir_;
    std::string getChunkPath(const std::string& raw_hash) const;
    std::string getManifestPath(const std::string& manifest_id) const;
};

// HTTP MinIO / S3 REST CAS store via libcurl
class HttpMinioCASRemote : public ICASRemoteStorage {
public:
    HttpMinioCASRemote(const std::string& endpointUrl,
                       const std::string& bucketName,
                       const std::string& accessKey = "",
                       const std::string& secretKey = "");

    bool hasChunk(const std::string& raw_hash) override;
    std::vector<std::string> queryMissingChunks(const std::vector<std::string>& raw_hashes) override;
    bool putChunk(const std::string& raw_hash, const std::vector<uint8_t>& payload) override;
    bool getChunk(const std::string& raw_hash, std::vector<uint8_t>& out_payload) override;

    bool putManifest(const std::string& manifest_id, const std::string& manifest_json) override;
    bool getManifest(const std::string& manifest_id, std::string& out_manifest_json) override;

    std::string getRemoteUri() const override { return endpointUrl_ + "/" + bucketName_; }
    std::string getRemoteType() const override { return "HttpMinioCAS"; }

private:
    std::string endpointUrl_;
    std::string bucketName_;
    std::string accessKey_;
    std::string secretKey_;

    std::string makeUrl(const std::string& subpath) const;
};

// Factory helper
std::unique_ptr<ICASRemoteStorage> createCASRemote(const std::string& endpointOrPath,
                                                  const std::string& bucketName = "aigit-cas");

// ---------------------------------------------------------------------------
// Global CAS Sync Engine
// ---------------------------------------------------------------------------
class CASSyncEngine {
public:
    CASSyncEngine() = default;

    // Explicit format-specific sync methods
    SyncResult syncSafeTensorsModelWithRemoteCAS(const std::string& filePath, ICASRemoteStorage& remote);
    SyncResult syncParquetDatasetWithRemoteCAS(const std::string& filePath, ICASRemoteStorage& remote);
    SyncResult syncMediaBlobWithRemoteCAS(const std::string& filePath, ICASRemoteStorage& remote);
    SyncResult syncGenericFileWithRemoteCAS(const std::string& filePath, ICASRemoteStorage& remote);

    // General format dispatcher
    SyncResult syncFileWithRemoteCAS(const std::string& filePath, ICASRemoteStorage& remote);

    // Explicit format-specific reconstruction / checkout methods
    bool reconstructSafeTensorsFromRemoteCAS(const FileManifest& manifest, const std::string& outputPath, ICASRemoteStorage& remote);
    bool reconstructParquetFromRemoteCAS(const FileManifest& manifest, const std::string& outputPath, ICASRemoteStorage& remote);
    bool reconstructMediaFromRemoteCAS(const FileManifest& manifest, const std::string& outputPath, ICASRemoteStorage& remote);
    bool reconstructGenericFromRemoteCAS(const FileManifest& manifest, const std::string& outputPath, ICASRemoteStorage& remote);

    // General reconstruction dispatcher by manifest ID
    bool reconstructFileFromRemoteCAS(const std::string& manifestId, const std::string& outputPath, ICASRemoteStorage& remote);

    // CAS Low-level operations
    std::vector<std::string> queryRemoteMissingChunks(ICASRemoteStorage& remote, const std::vector<std::string>& rawHashes);
    bool uploadChunkToRemoteCAS(ICASRemoteStorage& remote,
                                const std::string& rawHash,
                                const std::vector<uint8_t>& containerPayload);
    bool downloadChunkFromRemoteCAS(ICASRemoteStorage& remote,
                                    const std::string& rawHash,
                                    std::vector<uint8_t>& outRawData);

    FileManifest assembleRemoteManifest(const std::string& filePath,
                                        const std::vector<ChunkSyncRecord>& records,
                                        const std::string& formatName,
                                        const std::string& categoryName,
                                        const std::string& fileHash,
                                        size_t totalRawSize);
};

} // namespace Core::Sync
