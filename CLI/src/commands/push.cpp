//read all local hashes
//send a POST request with the chunks we have
//request reaches postgreSQL and it check the missing or modified chunks
//returns a presigned MinIo url for the missing chunks
//read the raw bytes of missing chunks 
//send an HTTP PUT request to minIO using libcurl

#include "push.hpp"
#include "../storage/metadata_db.hpp"
#include "../storage/object_store.hpp"

#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

namespace Commands
{
    std::vector<std::string> getLocalHashes(const Storage::MetadataDB& db)
    {
        return db.getAllObjectIds();
    }

    static std::map<std::string, std::string> extractUrlsFromJson(const std::string& json)
    {
        std::map<std::string, std::string> urlMap;
        size_t pos = 0;
        while ((pos = json.find("\"", pos)) != std::string::npos)
        {
            size_t keyEnd = json.find("\"", pos + 1);
            if (keyEnd == std::string::npos) break;
            std::string key = json.substr(pos + 1, keyEnd - pos - 1);

            size_t colon = json.find(":", keyEnd);
            if (colon == std::string::npos) break;

            size_t valStart = json.find("\"", colon);
            if (valStart == std::string::npos) break;
            size_t valEnd = json.find("\"", valStart + 1);
            if (valEnd == std::string::npos) break;
            std::string val = json.substr(valStart + 1, valEnd - valStart - 1);

            if (key.length() == 64)
            {
                urlMap[key] = val;
            }
            pos = valEnd + 1;
        }
        return urlMap;
    }

    std::string talkWithBackend(const std::string& serverUrl, const std::vector<std::string>& hashes)
    {
        std::string jsonPayload = "{\"chunks\": [";
        for (size_t i = 0; i < hashes.size(); ++i)
        {
            jsonPayload += "\"" + hashes[i] + "\"";
            if (i + 1 < hashes.size()) jsonPayload += ", ";
        }
        jsonPayload += "]}";

        CURL* curl = curl_easy_init();
        std::string response;
        if (!curl) return response;

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        std::string endpoint = serverUrl + "/api/v1/push/negotiate";
        curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* ptr, size_t size, size_t nmemb, std::string* data) -> size_t {
            data->append(static_cast<char*>(ptr), size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return response;
    }

    bool uploadToMinIO(const std::string& presignedUrl, const std::string& rawBytes)
    {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

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
        if (hashes.empty()) return true;

        std::string responseJson = talkWithBackend(serverUrl, hashes);

        std::map<std::string, std::string> missingChunks = extractUrlsFromJson(responseJson);

        for (const auto& [hash, url] : missingChunks)
        {
            std::string rawBytes = objectStore.retrieve(hash);
            if (!uploadToMinIO(url, rawBytes))
            {
                std::cerr << "Failed to upload chunk: " << hash << std::endl;
                curl_global_cleanup();
                return false;
            }
        }

        curl_global_cleanup();
        std::cout << "Push completed successfully." << std::endl;
        return true;
    }
}