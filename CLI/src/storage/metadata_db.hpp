#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Storage
{

struct ObjectMetadata
{
    std::string objectId;
    long long size;
    std::string type;
    std::string createdAt;
};

class MetadataDB
{
private:
    std::filesystem::path dbPath;

public:
    explicit MetadataDB(const std::filesystem::path& path);

    void initialize();

    void addObject(
        const std::string& objectId,
        long long size,
        const std::string& type
    );

    bool objectExists(
        const std::string& objectId
    ) const;

    ObjectMetadata getObject(
        const std::string& objectId
    ) const;

    std::vector<std::string> getAllObjectIds() const;

    struct RebuildResult
    {
        std::size_t objectsRebuilt;
        std::vector<std::string> corruptObjectIds;
    };

    // Rebuilds this DB's `objects` table from scratch by walking every
    // object under `casRoot`'s objects/ directory (ObjectStore::walkAll),
    // verifying each against its own ID and reading its type from the
    // on-disk header. Used to recover metadata.db after loss/corruption,
    // or as the data source for an fsck/gc entry point. Objects that fail
    // verification are reported in the result rather than aborting the
    // rebuild, so one bad object doesn't hide the rest.
    RebuildResult rebuild(const std::filesystem::path& casRoot);
};

}