#include "curl_helpers.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>

namespace Utils
{

std::size_t curlWriteToString(
    void* contents,
    std::size_t size,
    std::size_t nmemb,
    std::string* out
)
{
    std::size_t totalBytes = size * nmemb;
    out->append(static_cast<char*>(contents), totalBytes);
    return totalBytes;
}

std::size_t curlWriteToFile(
    void* contents,
    std::size_t size,
    std::size_t nmemb,
    std::ofstream* out
)
{
    std::size_t totalBytes = size * nmemb;
    out->write(
        static_cast<char*>(contents),
        static_cast<std::streamsize>(totalBytes)
    );
    return totalBytes;
}

std::string httpGet(const std::string& url, const std::string& logTag)
{
    std::string response;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr
            << "[" << logTag << "] Network Error: Could not reach server at "
            << url << " (" << curl_easy_strerror(res) << ")"
            << std::endl;
        return "";
    }

    return response;
}

std::string httpPostJson(
    const std::string& url,
    const std::string& jsonBody,
    const std::string& logTag
)
{
    std::string response;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return "";
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        std::cerr
            << "[" << logTag << "] Network Error: Could not reach server at "
            << url << " (" << curl_easy_strerror(res) << ")"
            << std::endl;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response;
}

std::map<std::string, std::string> parseHashUrlMap(
    const std::string& json,
    const std::string& key,
    const std::string& logTag
)
{
    std::map<std::string, std::string> urlMap;

    nlohmann::json parsed = nlohmann::json::parse(json, nullptr, false);

    if (parsed.is_discarded() || !parsed.is_object())
    {
        std::cerr << "[" << logTag << "] Error: Server returned malformed JSON." << std::endl;
        return urlMap;
    }

    if (!parsed.contains(key) || !parsed[key].is_object())
    {
        return urlMap;
    }

    for (const auto& [hash, url] : parsed[key].items())
    {
        if (url.is_string())
        {
            urlMap[hash] = url.get<std::string>();
        }
    }

    return urlMap;
}

bool downloadObjectToPath(
    const std::string& url,
    const std::filesystem::path& objectPath
)
{
    std::filesystem::create_directories(objectPath.parent_path());

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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteToFile);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fileOnDisk);

    CURLcode result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fileOnDisk.close();

    return (result == CURLE_OK);
}

}