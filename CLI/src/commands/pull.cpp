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

    std::string jsonResponse;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_URL, pullEndpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Utils::curlWriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonResponse);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr
            << "[Pull] Network Error: Could not reach server at "
            << pullEndpoint << " (" << curl_easy_strerror(res) << ")"
            << std::endl;
        return "";
    }

    return jsonResponse;
}

// Parses {"status":"ok","download_urls":{hash: url, ...}} into a map.
// Returns an empty map on malformed JSON or a missing/empty field.
static std::map<std::string, std::string> parseManifestJson(
    const std::string& json
)
{
    std::map<std::string, std::string> urlMap;

    nlohmann::json parsed =
        nlohmann::json::parse(json, nullptr, false);

    if (parsed.is_discarded() || !parsed.is_object())
    {
        std::cerr << "[Pull] Error: Server returned malformed JSON." << std::endl;
        return urlMap;
    }

    if (!parsed.contains("download_urls") ||
        !parsed["download_urls"].is_object())
    {
        return urlMap;
    }

    for (const auto& [hash, url] : parsed["download_urls"].items())
    {
        if (url.is_string())
        {
            urlMap[hash] = url.get<std::string>();
        }
    }

    return urlMap;
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

// Streams one chunk from its presigned MinIO URL to its CAS path on disk.
static bool downloadChunkPull(
    const std::string& url,
    const fs::path& objectPath
)
{
    fs::create_directories(objectPath.parent_path());

    std::ofstream fileOnDisk(objectPath, std::ios::binary);
    if (!fileOnDisk.is_open())
    {
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        fileOnDisk.close();
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Utils::curlWriteToFile);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fileOnDisk);

    CURLcode result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fileOnDisk.close();

    return (result == CURLE_OK);
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
        parseManifestJson(jsonResponse);

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
        fs::path objectPath =
            fs::path(".aigit") / "objects" / hash.substr(0, 2) / hash.substr(2);

        if (downloadChunkPull(url, objectPath))
        {
            long long fileSize =
                static_cast<long long>(fs::file_size(objectPath));

            metadataDB.addObject(hash, fileSize, "chunk");

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