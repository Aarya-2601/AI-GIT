#include "object_store.hpp"

#include "../core/hashing.hpp"
#include "../core/filesystem.hpp"
#include "../core/compression.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace Storage
{

namespace
{

// 4-byte magic marks the new header'd format so retrieve() can tell it
// apart from a legacy object (pre-header, raw bytes, no magic) without
// ambiguity -- a legacy blob/chunk happening to start with these exact
// four bytes is not realistically possible.
constexpr std::array<char, 4> kMagic = {'A', 'G', 'C', '1'};

constexpr size_t kHeaderSize =
    4 +  // magic
    1 +  // type
    1 +  // flags
    8;   // uncompressed size (uint64, little-endian)

constexpr uint8_t kFlagCompressed = 0x01;

// Below this size, compressing a full chunk/file is cheap enough that
// there's no need to pre-check with a sample.
constexpr size_t kBigInputThreshold = 256 * 1024;

constexpr size_t kSampleSize = 64 * 1024;

// If the compressed size isn't at least this fraction smaller than the
// original, it isn't worth storing compressed.
constexpr double kMinSavingsFraction = 0.05;

uint8_t typeToByte(const std::string& type)
{
    if (type == "file")     return 1;
    if (type == "chunk")    return 2;
    if (type == "manifest") return 3;
    if (type == "tree")     return 4;
    if (type == "commit")   return 5;

    return 0; // unknown
}

std::string byteToType(uint8_t typeByte)
{
    switch (typeByte)
    {
        case 1: return "file";
        case 2: return "chunk";
        case 3: return "manifest";
        case 4: return "tree";
        case 5: return "commit";
        default: return "unknown";
    }
}

// Legacy (pre-header) objects carry no type byte, so migration has to
// guess from content. Trees/commits are unambiguous: they're wrapped in
// a literal "tree <n>\0"/"commit <n>\0" prefix (see Models::Tree/Commit
// ::serialize). Manifests are JSON with a "type":"manifest" field. A
// blob and a chunk are byte-for-byte indistinguishable raw content, so
// both fall back to "unknown" -- harmless, since MetadataDB already
// records the real type separately whenever an object is written through
// the normal add/commit path; this header byte is only ever a disk-walk
// hint (rebuild/fsck), never load-bearing for retrieval.
std::string sniffLegacyType(const std::string& data)
{
    if (data.rfind("tree ", 0) == 0)
    {
        return "tree";
    }

    if (data.rfind("commit ", 0) == 0)
    {
        return "commit";
    }

    if (
        data.size() < 1024 * 1024 &&
        data.find("\"type\"") != std::string::npos &&
        data.find("\"manifest\"") != std::string::npos
    )
    {
        return "manifest";
    }

    return "unknown";
}

void appendUint64LE(std::string& out, uint64_t value)
{
    for (int i = 0; i < 8; ++i)
    {
        out.push_back(
            static_cast<char>((value >> (8 * i)) & 0xFF)
        );
    }
}

uint64_t readUint64LE(const char* bytes)
{
    uint64_t value = 0;

    for (int i = 0; i < 8; ++i)
    {
        value |=
            static_cast<uint64_t>(
                static_cast<unsigned char>(bytes[i])
            ) << (8 * i);
    }

    return value;
}

bool looksSufficientlyCompressible(
    const std::string& sampleOrWhole,
    const std::string& compressed
)
{
    return static_cast<double>(compressed.size()) <
        static_cast<double>(sampleOrWhole.size()) *
        (1.0 - kMinSavingsFraction);
}

// A suffix for temp files that's unique enough to avoid collisions
// between concurrent writers of different objects (each call gets its
// own tmp path, so no two writers ever share one).
std::string uniqueTmpSuffix()
{
    static thread_local std::mt19937_64 rng(
        std::hash<std::thread::id>{}(std::this_thread::get_id()) ^
        static_cast<uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()
        )
    );

    return std::to_string(rng());
}

}

ObjectStore::ObjectStore(
    const std::filesystem::path& root
)
    : rootPath(root)
{
}



void ObjectStore::initialize()
{

    std::filesystem::create_directories(
        rootPath / "objects"
    );
}


bool ObjectStore::exists(
    const std::string& objectId
) const
{
    if (objectId.length() < 2)
    {
        return false;
    }

    std::filesystem::path objectPath =
        rootPath /
        "objects" /
        objectId.substr(0, 2) /
        objectId.substr(2);

    return std::filesystem::exists(
        objectPath
    );
}


bool ObjectStore::remove(
    const std::string& objectId
)
{
    if (objectId.length() < 2)
    {
        return false;
    }

    std::filesystem::path objectPath =
        rootPath /
        "objects" /
        objectId.substr(0, 2) /
        objectId.substr(2);

    std::error_code ec;
    return std::filesystem::remove(objectPath, ec);
}


std::string ObjectStore::store(
    const std::filesystem::path& filePath
)
{
    std::string fileData =
        Core::readFileToString(filePath);


    // Calculate SHA-256 of the raw bytes.

    std::string objectId =
        Core::calcSHA256(
            fileData
        );


    if (objectId.empty())
    {
        throw std::runtime_error(
            "SHA-256 hashing failed."
        );
    }


    storeObject(
        objectId,
        fileData,
        "file"
    );


    return objectId;
}


void ObjectStore::storeObject(
    const std::string& objectId,
    const std::string& data,
    const std::string& type,
    int level
)
{
    if (objectId.length() < 2)
    {
        throw std::runtime_error(
            "Invalid object ID."
        );
    }

    std::filesystem::path objectPath =
        rootPath /
        "objects" /
        objectId.substr(0, 2) /
        objectId.substr(2);



    if (std::filesystem::exists(
            objectPath))
    {
        return;
    }

    writeObjectFileAtomic(objectPath, data, type, level);
}

void ObjectStore::writeObjectFileAtomic(
    const std::filesystem::path& objectPath,
    const std::string& data,
    const std::string& type,
    int level
)
{
    std::filesystem::create_directories(
        objectPath.parent_path()
    );


    // Decide whether compressing is worth it. For big inputs, first
    // compress just a 64KB sample -- if that alone doesn't clear the
    // savings bar, skip compressing the whole thing (cheap insurance
    // against paying full deflate cost on already-incompressible data).

    bool attemptFullCompression = !data.empty();

    if (attemptFullCompression && data.size() > kBigInputThreshold)
    {
        std::string sample = data.substr(0, kSampleSize);
        std::string sampleCompressed = Core::compressString(sample, level);

        if (!looksSufficientlyCompressible(sample, sampleCompressed))
        {
            attemptFullCompression = false;
        }
    }

    bool compressed = false;
    const std::string* payload = &data;
    std::string compressedData;

    if (attemptFullCompression)
    {
        compressedData = Core::compressString(data, level);

        if (looksSufficientlyCompressible(data, compressedData))
        {
            compressed = true;
            payload = &compressedData;
        }
    }

    std::string header;
    header.reserve(kHeaderSize);
    header.append(kMagic.data(), kMagic.size());
    header.push_back(static_cast<char>(typeToByte(type)));
    header.push_back(
        static_cast<char>(compressed ? kFlagCompressed : 0)
    );
    appendUint64LE(header, static_cast<uint64_t>(data.size()));


    // Write to a temp file in the same directory, then rename it into
    // place. A crash or interruption mid-write leaves only an orphaned
    // .tmp file, never a torn object at the final path -- readers never
    // see a partially-written object.

    std::filesystem::path tmpPath = objectPath;
    tmpPath += ".tmp";
    tmpPath += uniqueTmpSuffix();

    {
        std::ofstream output(
            tmpPath,
            std::ios::binary | std::ios::trunc
        );

        if (!output.is_open())
        {
            throw std::runtime_error(
                "Could not create CAS object: " +
                objectPath.string()
            );
        }

        output.write(header.data(), static_cast<std::streamsize>(header.size()));

        output.write(
            payload->data(),
            static_cast<std::streamsize>(
                payload->size()
            )
        );

        if (!output)
        {
            output.close();
            std::filesystem::remove(tmpPath);

            throw std::runtime_error(
                "Failed to write CAS object."
            );
        }
    }

    std::error_code renameEc;
    std::filesystem::rename(tmpPath, objectPath, renameEc);

    if (renameEc)
    {
        std::filesystem::remove(tmpPath);

        throw std::runtime_error(
            "Failed to finalize CAS object: " +
            objectPath.string()
        );
    }
}


std::string ObjectStore::retrieve(
    const std::string& objectId,
    std::string* typeOut
) const
{
    if (typeOut)
    {
        // Legacy (pre-header) objects carry no type byte on disk; only
        // overwritten below once a header'd object actually verifies.
        *typeOut = "unknown";
    }

    if (objectId.length() < 2)
    {
        throw std::runtime_error(
            "Invalid object ID."
        );
    }


    std::filesystem::path objectPath =
        rootPath /
        "objects" /
        objectId.substr(0, 2) /
        objectId.substr(2);


    if (!std::filesystem::exists(
            objectPath))
    {
        throw std::runtime_error(
            "Object not found: " +
            objectId
        );
    }


    std::string raw = Core::readFileToString(objectPath);

    // Legacy object: too short to even hold a header, or doesn't start
    // with the magic at all. Two legacy writers exist: blobs/chunks/
    // manifests written by the pre-step-2 ObjectStore (raw bytes on disk,
    // ID = SHA-256(raw) -- step 1's scheme), and trees/commits written by
    // the old Core::Storage path (zlib-compressed on disk, ID = SHA-256 of
    // the *uncompressed* payload). Try the cheap raw-match first, since
    // it covers the common case; only pay for a decompress attempt if
    // that fails. Only throw "corrupt" if neither interpretation matches.

    if (
        raw.size() < kHeaderSize ||
        std::memcmp(raw.data(), kMagic.data(), kMagic.size()) != 0
    )
    {
        if (Core::calcSHA256(raw) == objectId)
        {
            return raw;
        }

        std::string decompressed = Core::decompressData(raw);

        if (!decompressed.empty() && Core::calcSHA256(decompressed) == objectId)
        {
            return decompressed;
        }

        throw std::runtime_error(
            "Corrupt object (content does not match its ID): " +
            objectId
        );
    }

    // The file starts with the magic bytes, but that alone doesn't prove
    // it's actually a new-format object -- a legacy (pre-header) object's
    // raw content could coincidentally start with "AGC1". Content
    // addressing gives us a way to tell the two apart unambiguously: try
    // decoding it as a new-format object, and only trust that decoding
    // if the result actually hashes to objectId. If it doesn't, fall
    // back to treating the whole file as legacy raw content; if *that*
    // doesn't hash to objectId either, the object is genuinely corrupt.

    uint8_t flags = static_cast<uint8_t>(raw[5]);
    uint64_t uncompressedSize = readUint64LE(raw.data() + 6);

    std::string headerPayload = raw.substr(kHeaderSize);

    bool decodedOk = true;
    std::string decoded;

    if (flags & kFlagCompressed)
    {
        decoded = Core::decompressData(headerPayload);

        if (decoded.empty() && !headerPayload.empty())
        {
            decodedOk = false;
        }
    }
    else
    {
        decoded = headerPayload;
    }

    if (
        decodedOk &&
        decoded.size() == uncompressedSize &&
        Core::calcSHA256(decoded) == objectId
    )
    {
        if (typeOut)
        {
            *typeOut = byteToType(static_cast<uint8_t>(raw[4]));
        }

        return decoded;
    }

    // Not a valid new-format object for this ID. Check whether it's a
    // legacy object whose raw bytes just happen to start with "AGC1".

    if (Core::calcSHA256(raw) == objectId)
    {
        return raw;
    }

    throw std::runtime_error(
        "Corrupt object (content does not match its ID): " +
        objectId
    );
}

std::vector<ObjectStore::ObjectRecord> ObjectStore::walkAll(
    std::vector<std::string>* corruptObjectIds
) const
{
    std::vector<ObjectRecord> records;

    std::filesystem::path objectsRoot = rootPath / "objects";

    if (!std::filesystem::exists(objectsRoot))
    {
        return records;
    }

    for (
        const auto& dirEntry :
        std::filesystem::directory_iterator(objectsRoot)
    )
    {
        if (
            !dirEntry.is_directory() ||
            dirEntry.path().filename().string().size() != 2
        )
        {
            continue;
        }

        std::string prefix = dirEntry.path().filename().string();

        for (
            const auto& fileEntry :
            std::filesystem::directory_iterator(dirEntry.path())
        )
        {
            if (!fileEntry.is_regular_file())
            {
                continue;
            }

            std::string suffix = fileEntry.path().filename().string();

            // Skip orphaned .tmp* files left by an interrupted write --
            // not a finished object, so not part of the CAS content.
            if (suffix.find(".tmp") != std::string::npos)
            {
                continue;
            }

            std::string objectId = prefix + suffix;

            try
            {
                std::string type;
                std::string data = retrieve(objectId, &type);

                records.push_back(
                    {
                        objectId,
                        type,
                        static_cast<long long>(data.size())
                    }
                );
            }
            catch (const std::exception&)
            {
                if (corruptObjectIds)
                {
                    corruptObjectIds->push_back(objectId);
                }
            }
        }
    }

    return records;
}

ObjectStore::MigrationResult ObjectStore::migrateLegacyObjects(int level)
{
    MigrationResult result{};

    std::filesystem::path objectsRoot = rootPath / "objects";

    if (!std::filesystem::exists(objectsRoot))
    {
        return result;
    }

    for (
        const auto& dirEntry :
        std::filesystem::directory_iterator(objectsRoot)
    )
    {
        if (
            !dirEntry.is_directory() ||
            dirEntry.path().filename().string().size() != 2
        )
        {
            continue;
        }

        std::string prefix = dirEntry.path().filename().string();

        for (
            const auto& fileEntry :
            std::filesystem::directory_iterator(dirEntry.path())
        )
        {
            if (!fileEntry.is_regular_file())
            {
                continue;
            }

            std::string suffix = fileEntry.path().filename().string();

            if (suffix.find(".tmp") != std::string::npos)
            {
                continue;
            }

            std::string objectId = prefix + suffix;
            std::filesystem::path objectPath = fileEntry.path();

            std::string raw;

            try
            {
                raw = Core::readFileToString(objectPath);
            }
            catch (const std::exception&)
            {
                result.failedObjectIds.push_back(objectId);
                continue;
            }

            // Already header'd for this ID (same disambiguation retrieve()
            // uses: magic prefix alone isn't proof, only a hash match is).
            bool alreadyCurrent = false;

            if (
                raw.size() >= kHeaderSize &&
                std::memcmp(raw.data(), kMagic.data(), kMagic.size()) == 0
            )
            {
                uint8_t flags = static_cast<uint8_t>(raw[5]);
                uint64_t uncompressedSize = readUint64LE(raw.data() + 6);
                std::string headerPayload = raw.substr(kHeaderSize);

                std::string decoded = (flags & kFlagCompressed)
                    ? Core::decompressData(headerPayload)
                    : headerPayload;

                if (
                    decoded.size() == uncompressedSize &&
                    Core::calcSHA256(decoded) == objectId
                )
                {
                    alreadyCurrent = true;
                }
            }

            if (alreadyCurrent)
            {
                ++result.objectsAlreadyCurrent;
                continue;
            }

            // Not (validly) header'd -- must be a legacy object. Get its
            // verified canonical bytes the same way any other reader
            // would (retrieve() already knows how to interpret both
            // legacy raw and legacy zlib-wrapped content).
            std::string data;

            try
            {
                data = retrieve(objectId);
            }
            catch (const std::exception&)
            {
                result.failedObjectIds.push_back(objectId);
                continue;
            }

            std::string type = sniffLegacyType(data);

            writeObjectFileAtomic(objectPath, data, type, level);

            ++result.objectsMigrated;
        }
    }

    return result;
}

}