#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Storage
{

class ObjectStore
{
private:
    std::filesystem::path rootPath;

public:
    explicit ObjectStore(
        const std::filesystem::path& root
    );


    void initialize();

    bool exists(
        const std::string& objectId
    ) const;

    std::string store(
        const std::filesystem::path& filePath
    );


    // `type` is one of "file", "chunk", "manifest", "tree", "commit"
    // (matching MetadataDB's type strings); anything else is stored as
    // "unknown". It's recorded in the object's on-disk header for future
    // use (e.g. rebuilding MetadataDB from disk) and never affects the
    // object ID. `level` is the zlib compression level (1-9); the design
    // targets 1-3 for CAS objects.
    //
    // Compresses `data` and writes it with a small header (magic, type,
    // compressed flag, uncompressed size). If compression saves less than
    // 5% (or `data` is empty), the object is stored raw instead, with the
    // compressed flag cleared. Existing objects already on disk (written
    // before this header existed) are left untouched -- retrieve() below
    // still reads them correctly.
    void storeObject(
        const std::string& objectId,
        const std::string& data,
        const std::string& type = "",
        int level = 2
    );

    // Retrieves the raw (decompressed) bytes of an object using its
    // SHA-256 object ID. Transparently reads both the new header'd
    // format and legacy pre-header objects (raw bytes, no header) so
    // old repositories remain readable without a migration step.
    //
    // If `typeOut` is non-null, it's set to the object's type as recorded
    // in its on-disk header ("file"/"chunk"/"manifest"/"tree"/"commit"/
    // "unknown"), or "unknown" for a legacy (pre-header) object, which
    // carries no type byte at all.
    std::string retrieve(
        const std::string& objectId,
        std::string* typeOut = nullptr
    ) const;

    struct ObjectRecord
    {
        std::string objectId;
        std::string type;
        long long size;
    };

    // Walks every object under objects/, verifying each one against its
    // own ID (the same check retrieve() performs) and reading its type
    // from the on-disk header. An object that fails verification is
    // skipped here and its ID appended to `corruptObjectIds` instead of
    // aborting the whole walk -- used by fsck/MetadataDB::rebuild, where
    // one bad object on disk shouldn't hide the rest.
    std::vector<ObjectRecord> walkAll(
        std::vector<std::string>* corruptObjectIds = nullptr
    ) const;
};

}