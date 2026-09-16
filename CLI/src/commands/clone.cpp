// Clone flow:
// 1. Set up a fresh .aigit directory (objects/ + metadata.db).
// 2. GET the target repo's full manifest (hash -> presigned download URL)
//    from the backend.
// 3. Download every chunk via its presigned MinIO URL.
// 4. Register each chunk in the local MetadataDB.
// 5. Remember the repo name in .aigit/config so later `ai-git pull` with
//    no arguments knows what to pull.

#include "clone.hpp"
#include "../storage/metadata_db.hpp"
#include "../storage/object_store.hpp"
#include "../core/config.hpp"
#include "../core/filesystem.hpp"
#include "../helpers/curl_helpers.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <map>

namespace fs = std::filesystem;

namespace Commands
{

static std::string getManifestClone(
    const std::string& serverUrl,
    const std::string& repoName
)
{
    std::string cloneEndpoint =
        serverUrl + "/api/v1/pull/clone/" + repoName;

    std::string jsonResponse;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_URL, cloneEndpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Utils::curlWriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonResponse);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr
            << "[Clone] Network Error: Failed to contact backend at "
            << cloneEndpoint << " (" << curl_easy_strerror(res) << ")"
            << std::endl;
        return "";
    }

    return jsonResponse;
}

static std::map<std::string, std::string> parseManifestJson(
    const std::string& json
)
{
    std::map<std::string, std::string> urlMap;

    nlohmann::json parsed =
        nlohmann::json::parse(json, nullptr, false);

    if (parsed.is_discarded() || !parsed.is_object())
    {
        std::cerr << "[Clone] Error: Server returned malformed JSON." << std::endl;
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

static bool downloadChunkClone(
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

bool runClone(const std::string& reponame, const std::string& server)
{
    if (fs::exists(".aigit"))
    {
        std::cerr
            << "[Clone] Error: '.aigit' already exists in this directory."
            << std::endl;
        return false;
    }

    std::cout
        << "[Clone] Setting up local .aigit directory for '"
        << reponame << "'..." << std::endl;

    curl_global_init(CURL_GLOBAL_ALL);

    Storage::ObjectStore objectStore(".aigit");
    objectStore.initialize();

    Storage::MetadataDB metadataDB(".aigit/metadata.db");
    metadataDB.initialize();

    std::string jsonResponse = getManifestClone(server, reponame);
    if (jsonResponse.empty())
    {
        curl_global_cleanup();
        return false;
    }

    std::map<std::string, std::string> downloadMap =
        parseManifestJson(jsonResponse);

    if (downloadMap.empty())
    {
        std::cerr
            << "[Clone] Error: No chunks returned or repository '"
            << reponame << "' not found." << std::endl;
        curl_global_cleanup();
        return false;
    }

    std::cout
        << "[Clone] Downloading " << downloadMap.size()
        << " chunk(s) from MinIO..." << std::endl;

    std::size_t successCount = 0;

    for (const auto& [hash, url] : downloadMap)
    {
        fs::path objectPath =
            fs::path(".aigit") / "objects" / hash.substr(0, 2) / hash.substr(2);

        if (downloadChunkClone(url, objectPath))
        {
            long long fileSize =
                static_cast<long long>(fs::file_size(objectPath));

            metadataDB.addObject(hash, fileSize, "chunk");

            std::cout
                << "Downloaded chunk: " << hash.substr(0, 8) << "..."
                << std::endl;
            ++successCount;
        }
        else
        {
            std::cerr
                << "Failed to download chunk: " << hash.substr(0, 8) << "..."
                << std::endl;
        }
    }

    std::cout
        << "[Clone] Complete! (" << successCount << "/"
        << downloadMap.size() << " chunks saved)" << std::endl;

    // Remember which repo this working copy tracks so that a bare
    // `ai-git pull` (no args) later knows what to pull.
    Core::Config config;
    config.load(".aigit/config");
    config.set("remote.repo", reponame);
    config.save(".aigit/config");

    curl_global_cleanup();
    return successCount == downloadMap.size();
}

}