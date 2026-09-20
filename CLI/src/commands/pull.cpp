// Pull flow:
// 1. Work out which remote repo we're tracking (saved in .aigit/config
//    under [remote] repo = <name> the first time you `clone`).
// 2. GET the repo's manifest (hash -> presigned download URL) from the
//    backend.
// 3. Diff that against what we already have locally (MetadataDB).
// 4. Download only the missing chunks from MinIO via the presigned URLs.
// 5. Register each newly-downloaded chunk in the local MetadataDB.

#include "pull.hpp"
#include "../storage/metadata_db.hpp"
#include "../storage/object_store.hpp"
#include "../core/config.hpp"
#include "../core/hashing.hpp"
#include "../helpers/curl_helpers.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

namespace Commands
{

// Sends a GET request for the repo manifest and returns the raw JSON body.
// Empty string means the request itself failed (network/server error) --
// that's different from "repo has no chunks", which is an empty JSON map.
static std::string getManifestPull(
    const std::string& serverUrl,
    const std::string& repoName
)
{
    std::string pullEndpoint =
        serverUrl + "/api/v1/pull/clone/" + repoName;

    return Utils::httpGet(pullEndpoint, "Pull");
}

// Keep only the (hash, url) pairs we don't already have in local CAS.
static std::map<std::string, std::string> filterChunks(
    const std::map<std::string, std::string>& remoteManifest,
    const Storage::MetadataDB& metadataDB
)
{
    std::map<std::string, std::string> missingChunks;

    for (const auto& [hash, url] : remoteManifest)
    {
        if (!metadataDB.objectExists(hash))
        {
            missingChunks[hash] = url;
        }
    }

    return missingChunks;
}

bool runPull(const std::string& reponame, const std::string& server)
{
    // Resolve which repo to pull *before* touching curl/metadataDB, since
    // it may come from .aigit/config rather than the argument.
    std::string repoToPull = reponame;

    if (repoToPull.empty() || repoToPull == "default-repo")
    {
        Core::Config config;
        config.load(".aigit/config");

        repoToPull = config.get("remote.repo");

        if (repoToPull.empty())
        {
            std::cerr
                << "[Error] Could not determine remote repository name. "
                << "Please specify it or run 'ai-git clone <repo>' first."
                << std::endl;
            return false;
        }
    }

    std::cout
        << "[Pull] Checking remote repository '"
        << repoToPull << "' for updates..." << std::endl;

    curl_global_init(CURL_GLOBAL_ALL);

    Storage::MetadataDB metadataDB(".aigit/metadata.db");
    Storage::ObjectStore objectStore(".aigit");

    std::string jsonResponse = getManifestPull(server, repoToPull);
    if (jsonResponse.empty())
    {
        curl_global_cleanup();
        return false;
    }

    std::map<std::string, std::string> remoteManifest =
        Utils::parseHashUrlMap(jsonResponse, "download_urls", "Pull");

    if (remoteManifest.empty())
    {
        std::cerr
            << "[Pull] Remote repository is empty or not found."
            << std::endl;
        curl_global_cleanup();
        return false;
    }

    std::map<std::string, std::string> missingChunks =
        filterChunks(remoteManifest, metadataDB);

    if (missingChunks.empty())
    {
        std::cout
            << "[Pull] Local repository is already up to date."
            << std::endl;
        curl_global_cleanup();
        return true;
    }

    std::cout
        << "[Pull] Found " << missingChunks.size()
        << " new chunk(s) to pull..." << std::endl;

    std::size_t successCount = 0;

    for (const auto& [hash, url] : missingChunks)
    {
        std::string data;

        if (
            Utils::downloadObjectToString(url, data) &&
            Core::calcSHA256(data) == hash
        )
        {
            // Write through ObjectStore::storeObject (atomic tmp+rename,
            // header/compression) instead of a raw direct file write, so
            // downloaded chunks land on disk the same way locally-added
            // ones do.
            objectStore.storeObject(hash, data, "chunk");

            metadataDB.addObject(
                hash,
                static_cast<long long>(data.size()),
                "chunk"
            );

            std::cout
                << "Pulled chunk: " << hash.substr(0, 8) << "..."
                << std::endl;
            ++successCount;
        }
        else
        {
            std::cerr
                << "Failed to pull chunk: " << hash.substr(0, 8) << "..."
                << std::endl;
        }
    }

    std::cout
        << "[Pull] Complete! (" << successCount << "/"
        << missingChunks.size() << " updated)" << std::endl;

    curl_global_cleanup();
    return successCount == missingChunks.size();
}

}