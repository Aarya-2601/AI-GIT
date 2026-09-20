#include "object_store.hpp"

#include "../core/hashing.hpp"
#include "../core/compression.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace
{

std::string readWholeFile(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    std::string out(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>()
    );
    return out;
}

std::filesystem::path onDiskPathFor(
    const std::filesystem::path& root,
    const std::string& objectId
)
{
    return root / "objects" /
        objectId.substr(0, 2) / objectId.substr(2);
}

size_t countTmpFiles(const std::filesystem::path& objectsRoot)
{
    size_t count = 0;

    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(objectsRoot))
    {
        if (
            entry.is_regular_file() &&
            entry.path().string().find(".tmp") != std::string::npos
        )
        {
            ++count;
        }
    }

    return count;
}

}

int main()
{
    try
    {
        Storage::ObjectStore store(".aigit/cas");

        store.initialize();

        std::string id =
            store.store("test.txt");

        std::cout << "Object ID: "
                  << id
                  << "\n";

        std::cout << "Exists: "
                  << (store.exists(id)
                        ? "YES"
                        : "NO")
                  << "\n";

        std::string data =
            store.retrieve(id);

        std::cout << "Retrieved: "
                  << data
                  << "\n";

        // Regression: object IDs must be the SHA-256 of the raw content
        // bytes only -- no git-style "type <size>\0" wrapper, no header.
        // This is the invariant the whole repo's ID scheme is unified on
        // (add/status/commit/hash_object all need to agree on it), and
        // it must keep holding once compression/headers are added on top
        // in ObjectStore's on-disk representation.

        std::string rawId =
            Core::calcSHA256(data);

        if (rawId != id)
        {
            std::cerr
                << "FAILED: object ID is not the SHA-256 of the raw "
                << "content bytes (expected "
                << rawId
                << ", got "
                << id
                << ").\n";

            return 1;
        }

        // storeObject/retrieve must round-trip arbitrary bytes exactly,
        // including embedded NUL bytes, unchanged.

        std::string binaryData(
            "\x00\x01binary\xff\x00payload",
            22
        );

        std::string binaryId =
            Core::calcSHA256(binaryData);

        store.storeObject(binaryId, binaryData);

        std::string roundTripped =
            store.retrieve(binaryId);

        if (roundTripped != binaryData)
        {
            std::cerr
                << "FAILED: storeObject/retrieve did not round-trip "
                << "binary data exactly.\n";

            return 1;
        }

        std::cout
            << "SUCCESS: object IDs are raw-content SHA-256 with no "
            << "wrapper, and storeObject/retrieve round-trips exactly.\n";

        // Highly compressible large ("chunk"-sized) input should be
        // stored compressed: on-disk size should be well below the
        // original, and retrieve() should transparently decompress back
        // to the exact original bytes.

        std::string compressible(300 * 1024, 'a');

        std::string compressibleId =
            Core::calcSHA256(compressible);

        store.storeObject(compressibleId, compressible, "chunk");

        std::string onDiskCompressible =
            readWholeFile(onDiskPathFor(".aigit/cas", compressibleId));

        if (onDiskCompressible.size() >= compressible.size())
        {
            std::cerr
                << "FAILED: highly compressible data was not stored "
                << "compressed (on-disk size " << onDiskCompressible.size()
                << " >= original " << compressible.size() << ").\n";

            return 1;
        }

        if (store.retrieve(compressibleId) != compressible)
        {
            std::cerr
                << "FAILED: compressed object did not decompress back "
                << "to the original bytes.\n";

            return 1;
        }

        // Incompressible (random) large input should fall back to raw
        // storage instead of paying compression cost for no benefit.

        std::string incompressible(300 * 1024, '\0');
        {
            std::mt19937 rng(42);
            std::uniform_int_distribution<int> dist(0, 255);
            for (char& c : incompressible)
            {
                c = static_cast<char>(dist(rng));
            }
        }

        std::string incompressibleId =
            Core::calcSHA256(incompressible);

        store.storeObject(incompressibleId, incompressible, "chunk");

        std::string onDiskIncompressible =
            readWholeFile(onDiskPathFor(".aigit/cas", incompressibleId));

        // Header is 14 bytes; raw fallback means on-disk size is exactly
        // header + original (no zlib overhead on top).
        if (onDiskIncompressible.size() != incompressible.size() + 14)
        {
            std::cerr
                << "FAILED: incompressible data did not fall back to raw "
                << "storage (on-disk size " << onDiskIncompressible.size()
                << ", expected " << (incompressible.size() + 14) << ").\n";

            return 1;
        }

        if (store.retrieve(incompressibleId) != incompressible)
        {
            std::cerr
                << "FAILED: raw-fallback object did not round-trip "
                << "exactly.\n";

            return 1;
        }

        // A legacy object (written with no header at all, as every
        // object was before this change) must still be readable and
        // returned byte-for-byte unchanged.

        std::string legacyData = "legacy object, no header";
        std::string legacyId = Core::calcSHA256(legacyData);
        std::filesystem::path legacyPath =
            onDiskPathFor(".aigit/cas", legacyId);

        std::filesystem::create_directories(legacyPath.parent_path());
        {
            std::ofstream legacyOut(legacyPath, std::ios::binary);
            legacyOut << legacyData;
        }

        if (store.retrieve(legacyId) != legacyData)
        {
            std::cerr
                << "FAILED: legacy (pre-header) object was not read back "
                << "unchanged.\n";

            return 1;
        }

        std::cout
            << "SUCCESS: compressible data is stored compressed, "
            << "incompressible data falls back to raw, and legacy "
            << "pre-header objects remain readable.\n";

        // A legacy object whose raw content happens to start with the
        // "AGC1" magic must still be read back unchanged, not misread
        // as a (garbage) new-format header.

        std::string legacyLookAlike =
            "AGC1 is not really a header, just legacy text data that "
            "happens to start with the magic bytes by coincidence.";
        std::string legacyLookAlikeId =
            Core::calcSHA256(legacyLookAlike);
        std::filesystem::path legacyLookAlikePath =
            onDiskPathFor(".aigit/cas", legacyLookAlikeId);

        std::filesystem::create_directories(
            legacyLookAlikePath.parent_path()
        );
        {
            std::ofstream out(legacyLookAlikePath, std::ios::binary);
            out << legacyLookAlike;
        }

        if (store.retrieve(legacyLookAlikeId) != legacyLookAlike)
        {
            std::cerr
                << "FAILED: legacy object starting with \"AGC1\" was not "
                << "read back unchanged.\n";

            return 1;
        }

        // A genuinely corrupted new-format object (payload tampered with
        // after being written) must be reported as corrupt, not silently
        // returned as if it were legacy or valid.

        std::string corruptSource(300 * 1024, 'b');
        std::string corruptId = Core::calcSHA256(corruptSource);

        store.storeObject(corruptId, corruptSource, "chunk");

        std::filesystem::path corruptPath =
            onDiskPathFor(".aigit/cas", corruptId);

        {
            std::fstream corruptFile(
                corruptPath,
                std::ios::binary | std::ios::in | std::ios::out
            );

            corruptFile.seekp(20);
            char flipped = 'X';
            corruptFile.write(&flipped, 1);
        }

        bool threw = false;
        try
        {
            store.retrieve(corruptId);
        }
        catch (const std::exception&)
        {
            threw = true;
        }

        if (!threw)
        {
            std::cerr
                << "FAILED: corrupted new-format object was not detected "
                << "as corrupt.\n";

            return 1;
        }

        std::cout
            << "SUCCESS: a legacy object starting with \"AGC1\" reads back "
            << "unchanged, and a corrupted new-format object is reported "
            << "as corrupt.\n";

        // A corrupted legacy (pre-header) object -- on-disk bytes no
        // longer match the ID they're stored under -- must also be
        // reported as corrupt, not silently returned.

        std::string legacyCorruptData = "legacy object that will be corrupted";
        std::string legacyCorruptId = Core::calcSHA256(legacyCorruptData);
        std::filesystem::path legacyCorruptPath =
            onDiskPathFor(".aigit/cas", legacyCorruptId);

        std::filesystem::create_directories(
            legacyCorruptPath.parent_path()
        );
        {
            std::ofstream out(legacyCorruptPath, std::ios::binary);
            out << legacyCorruptData;
        }
        {
            std::fstream legacyCorruptFile(
                legacyCorruptPath,
                std::ios::binary | std::ios::in | std::ios::out
            );

            legacyCorruptFile.seekp(0);
            char flipped = 'X';
            legacyCorruptFile.write(&flipped, 1);
        }

        bool legacyThrew = false;
        try
        {
            store.retrieve(legacyCorruptId);
        }
        catch (const std::exception&)
        {
            legacyThrew = true;
        }

        if (!legacyThrew)
        {
            std::cerr
                << "FAILED: corrupted legacy object was not detected as "
                << "corrupt.\n";

            return 1;
        }

        std::cout
            << "SUCCESS: a corrupted legacy object is also reported as "
            << "corrupt (verify-on-read covers both formats).\n";

        // Atomic write: after a normal storeObject(), no orphaned .tmp
        // file should be left behind in the CAS objects tree.

        std::string atomicData(500 * 1024, 'c');
        std::string atomicId = Core::calcSHA256(atomicData);

        store.storeObject(atomicId, atomicData, "chunk");

        size_t tmpFilesAfterWrite = countTmpFiles(".aigit/cas/objects");

        if (tmpFilesAfterWrite != 0)
        {
            std::cerr
                << "FAILED: " << tmpFilesAfterWrite
                << " orphaned .tmp file(s) left after a normal write.\n";

            return 1;
        }

        if (store.retrieve(atomicId) != atomicData)
        {
            std::cerr
                << "FAILED: object written via the atomic tmp+rename path "
                << "did not round-trip correctly.\n";

            return 1;
        }

        std::cout
            << "SUCCESS: storeObject leaves no orphaned .tmp files and "
            << "still round-trips correctly.\n";

        // --- Legacy tree/commit compatibility -----------------------------
        //
        // Pre-step-5 repos have tree/commit objects written by the old
        // Core::Storage path: zlib-compressed on disk (via
        // Core::compressString), no AGC1 header, ID = SHA-256 of the
        // *uncompressed* "tree <n>\0..."/"commit <n>\0..." payload (see
        // Models::Tree::serialize/Models::Commit::serialize). From
        // ObjectStore::retrieve()'s point of view this looks like a
        // "legacy, no header" object, but unlike legacy blobs/chunks
        // (raw bytes on disk, ID = SHA-256(raw)) its on-disk bytes are
        // compressed. retrieve()'s legacy branch tries SHA-256(raw) first
        // and, on mismatch, falls back to SHA-256(decompress(raw)) -- so
        // these fixtures must now read back correctly.

        std::string legacyCommitBody =
            "tree 0000000000000000000000000000000000000000000000000000000000000000\n"
            "author Test <test@example.com> 0 +0000\n"
            "committer Test <test@example.com> 0 +0000\n"
            "\n"
            "legacy commit fixture\n";

        std::string legacyCommitPayload =
            "commit " + std::to_string(legacyCommitBody.size()) +
            std::string(1, '\0') + legacyCommitBody;

        std::string legacyCommitId =
            Core::calcSHA256(legacyCommitPayload);

        std::string legacyCommitCompressed =
            Core::compressString(legacyCommitPayload);

        std::filesystem::path legacyCommitPath =
            onDiskPathFor(".aigit/cas", legacyCommitId);

        std::filesystem::create_directories(
            legacyCommitPath.parent_path()
        );
        {
            std::ofstream out(legacyCommitPath, std::ios::binary);
            out.write(
                legacyCommitCompressed.data(),
                static_cast<std::streamsize>(legacyCommitCompressed.size())
            );
        }

        std::string retrievedLegacyCommit =
            store.retrieve(legacyCommitId);

        if (retrievedLegacyCommit != legacyCommitPayload)
        {
            std::cerr
                << "FAIL: legacy zlib-compressed commit-style object did "
                << "not round-trip through ObjectStore::retrieve().\n";

            return 1;
        }

        std::cout
            << "SUCCESS: a legacy zlib-compressed commit-style object "
            << "round-trips through ObjectStore::retrieve() via the "
            << "decompress-then-hash fallback.\n";

        // Same coverage for a legacy TREE object (Models::Tree::serialize
        // format: "tree <n>\0" + repeated "<mode> <name>\0<20-byte hash>"
        // entries), not just commits -- both hit the same legacy
        // zlib-wrapped code path in retrieve(), but only commit was
        // covered above.

        std::string blobHashBytes(32, '\0');

        std::string legacyTreeContent;
        legacyTreeContent += "100644 fixture.txt";
        legacyTreeContent.push_back('\0');
        legacyTreeContent += blobHashBytes;

        std::string legacyTreePayload =
            "tree " + std::to_string(legacyTreeContent.size()) +
            std::string(1, '\0') + legacyTreeContent;

        std::string legacyTreeId =
            Core::calcSHA256(legacyTreePayload);

        std::string legacyTreeCompressed =
            Core::compressString(legacyTreePayload);

        std::filesystem::path legacyTreePath =
            onDiskPathFor(".aigit/cas", legacyTreeId);

        std::filesystem::create_directories(
            legacyTreePath.parent_path()
        );
        {
            std::ofstream out(legacyTreePath, std::ios::binary);
            out.write(
                legacyTreeCompressed.data(),
                static_cast<std::streamsize>(legacyTreeCompressed.size())
            );
        }

        std::string retrievedLegacyTree =
            store.retrieve(legacyTreeId);

        if (retrievedLegacyTree != legacyTreePayload)
        {
            std::cerr
                << "FAIL: legacy zlib-compressed tree-style object did "
                << "not round-trip through ObjectStore::retrieve().\n";

            return 1;
        }

        std::cout
            << "SUCCESS: a legacy zlib-compressed tree-style object "
            << "round-trips through ObjectStore::retrieve() via the "
            << "decompress-then-hash fallback.\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: "
                  << e.what()
                  << "\n";

        return 1;
    }

    return 0;
}