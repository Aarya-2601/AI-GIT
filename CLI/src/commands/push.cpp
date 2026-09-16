// Push flow:
// 1. Read every local object hash we know about (MetadataDB).
// 2. POST that hash list to the backend's negotiate endpoint.
// 3. Backend replies with a presigned MinIO upload URL per hash it wants
//    (ideally only the ones it doesn't already have -- see the backend
//    TODO in push.controller.js for the dedup check that still needs to
//    be added there).
// 4. PUT the raw bytes of each requested chunk to its presigned URL.

#include "push.hpp"
#include "../storage/metadata_db.hpp"
#include "../storage/object_store.hpp"
#include "../helpers/curl_helpers.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <map>

namespace Commands
{

static std::vector<std::string> getLocalHashes(const Storage::MetadataDB& db)
{
    return db.getAllObjectIds();
}

// Parses {"status":"ok","upload_urls":{hash: url, ...}} into a map.
static std::map<std::string, std::string> parseUploadUrls(const std::string& json)
{
    std::map<std::string, std::string> urlMap;

    nlohmann::json parsed = nlohmann::json::parse(json, nullptr, false);

    if (parsed.is_discarded() || !parsed.is_object())
    {
        std::cerr << "[Push] Error: Server returned malformed JSON." << std::endl;
        return urlMap;
    }

    if (!parsed.contains("upload_urls") || !parsed["upload_urls"].is_object())
    {
        return urlMap;
    }

    for (const auto& [hash, url] : parsed["upload_urls"].items())
    {
        if (url.is_string())
        {
            urlMap[hash] = url.get<std::string>();
        }
    }

    return urlMap;
}

static std::string talkWithBackend(
    const std::string& serverUrl,
    const std::vector<std::string>& hashes
)
{
    nlohmann::json payload;
    payload["chunks"] = hashes;

    std::string jsonPayload = payload.dump();
    std::string response;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return response;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    std::string endpoint = serverUrl + "/api/v1/push/negotiate";

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Utils::curlWriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        std::cerr
            << "[Push] Network Error: Could not reach server at "
            << endpoint << " (" << curl_easy_strerror(res) << ")"
            << std::endl;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response;
}

static bool uploadToMinIO(const std::string& presignedUrl, const std::string& rawBytes)
{
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return false;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");

    curl_easy_setopt(curl, CURLOPT_URL, presignedUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, rawBytes.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(rawBytes.size()));

    CURLcode res = curl_easy_perform(curl);

    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && responseCode >= 200 && responseCode < 300);
}

bool runPush(const std::string& serverUrl)
{
    curl_global_init(CURL_GLOBAL_ALL);

    Storage::MetadataDB metadataDB(".aigit/metadata.db");
    Storage::ObjectStore objectStore(".aigit");

    std::vector<std::string> hashes = getLocalHashes(metadataDB);
    if (hashes.empty())
    {
        std::cout << "[Push] Nothing to push." << std::endl;
        curl_global_cleanup();
        return true;
    }

    std::string responseJson = talkWithBackend(serverUrl, hashes);
    if (responseJson.empty())
    {
        curl_global_cleanup();
        return false;
    }

    std::map<std::string, std::string> uploadUrls = parseUploadUrls(responseJson);

    if (uploadUrls.empty())
    {
        std::cout << "[Push] Server has nothing new to receive." << std::endl;
        curl_global_cleanup();
        return true;
    }

    std::cout << "[Push] Uploading " << uploadUrls.size() << " object(s)..." << std::endl;

    for (const auto& [hash, url] : uploadUrls)
    {
        std::string rawBytes = objectStore.retrieve(hash);

        if (!uploadToMinIO(url, rawBytes))
        {
            std::cerr << "Failed to upload chunk: " << hash << std::endl;
            curl_global_cleanup();
            return false;
        }

        std::cout << "Uploaded: " << hash.substr(0, 8) << "..." << std::endl;
    }

    curl_global_cleanup();
    std::cout << "Push completed successfully." << std::endl;
    return true;
}

}