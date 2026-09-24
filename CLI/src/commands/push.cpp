// Push flow:
// 1. Read every local object hash tracked by MetadataDB.
// 2. Ask the backend which objects are missing.
// 3. Upload only the missing objects to MinIO.
// 4. Read the repository's HEAD and refs.
// 5. Finalize the push so the backend knows which commit belongs to this repo.

#include "push.hpp"
#include "../storage/metadata_db.hpp"
#include "../storage/object_store.hpp"
#include "../helpers/curl_helpers.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace Commands
{

namespace fs = std::filesystem;

static std::vector<std::string> getLocalHashes(
    const Storage::MetadataDB& db
)
{
    return db.getAllObjectIds();
}


static std::string readTextFile(
    const fs::path& path
)
{
    std::ifstream file(path);

    if (!file)
    {
        return "";
    }

    std::string value;
    std::getline(file, value);

    while (
        !value.empty() &&
        (value.back() == '\r' || value.back() == '\n')
    )
    {
        value.pop_back();
    }

    return value;
}


static std::string getRepositoryName()
{
    return fs::current_path().filename().string();
}


static std::string getHeadRef()
{
    std::string head =
        readTextFile(".aigit/HEAD");

    const std::string prefix = "ref: ";

    if (head.rfind(prefix, 0) != 0)
    {
        return "";
    }

    return head.substr(prefix.size());
}


static nlohmann::json getLocalRefs()
{
    nlohmann::json refs =
        nlohmann::json::object();

    const fs::path headsDir =
        ".aigit/refs/heads";

    if (!fs::exists(headsDir))
    {
        return refs;
    }


    for (
        const auto& entry :
        fs::recursive_directory_iterator(headsDir)
    )
    {
        if (!entry.is_regular_file())
        {
            continue;
        }


        fs::path relative =
            fs::relative(
                entry.path(),
                ".aigit"
            );


        std::string refName =
            relative.generic_string();


        std::string commitHash =
            readTextFile(entry.path());


        if (!commitHash.empty())
        {
            refs[refName] =
                commitHash;
        }
    }


    return refs;
}


static std::string talkWithBackend(
    const std::string& serverUrl,
    const std::vector<std::string>& hashes
)
{
    nlohmann::json payload;

    payload["chunks"] = hashes;


    const std::string endpoint =
        serverUrl +
        "/api/v1/push/negotiate";


    return Utils::httpPostJson(
        endpoint,
        payload.dump(),
        "Push"
    );
}


static bool uploadToMinIO(
    const std::string& presignedUrl,
    const std::string& rawBytes
)
{
    CURL* curl =
        curl_easy_init();

    if (!curl)
    {
        return false;
    }


    struct curl_slist* headers =
        nullptr;


    headers = curl_slist_append(
        headers,
        "Content-Type: application/octet-stream"
    );


    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        presignedUrl.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_CUSTOMREQUEST,
        "PUT"
    );

    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDS,
        rawBytes.data()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDSIZE,
        static_cast<long>(
            rawBytes.size()
        )
    );


    CURLcode result =
        curl_easy_perform(curl);


    long responseCode = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &responseCode
    );


    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);


    return (
        result == CURLE_OK &&
        responseCode >= 200 &&
        responseCode < 300
    );
}


static bool finalizePush(
    const std::string& serverUrl
)
{
    const std::string repoName =
        getRepositoryName();

    const std::string headRef =
        getHeadRef();

    const nlohmann::json refs =
        getLocalRefs();


    if (repoName.empty())
    {
        std::cerr
            << "[Push] Could not determine repository name."
            << std::endl;

        return false;
    }


    if (headRef.empty())
    {
        std::cerr
            << "[Push] Invalid or detached HEAD."
            << std::endl;

        return false;
    }


    if (!refs.contains(headRef))
    {
        std::cerr
            << "[Push] HEAD points to a branch with no commit."
            << std::endl;

        return false;
    }


    nlohmann::json payload;

    payload["repo"] =
        repoName;

    payload["head"] =
        headRef;

    payload["refs"] =
        refs;


    const std::string endpoint =
        serverUrl +
        "/api/v1/push/finalize";


    const std::string response =
        Utils::httpPostJson(
            endpoint,
            payload.dump(),
            "Push finalize"
        );


    return !response.empty();
}


bool runPush(
    const std::string& serverUrl
)
{
    curl_global_init(
        CURL_GLOBAL_ALL
    );


    Storage::MetadataDB metadataDB(
        ".aigit/metadata.db"
    );

    Storage::ObjectStore objectStore(
        ".aigit"
    );


    std::vector<std::string> hashes =
        getLocalHashes(metadataDB);


    if (hashes.empty())
    {
        std::cout
            << "[Push] Nothing to push."
            << std::endl;

        curl_global_cleanup();

        return true;
    }


    std::string responseJson =
        talkWithBackend(
            serverUrl,
            hashes
        );


    if (responseJson.empty())
    {
        curl_global_cleanup();

        return false;
    }


    std::map<std::string, std::string>
        uploadUrls =
            Utils::parseHashUrlMap(
                responseJson,
                "upload_urls",
                "Push"
            );


    if (uploadUrls.empty())
    {
        std::cout
            << "[Push] All objects already exist remotely."
            << std::endl;
    }
    else
    {
        std::cout
            << "[Push] Uploading "
            << uploadUrls.size()
            << " object(s)..."
            << std::endl;


        for (
            const auto& [hash, url] :
            uploadUrls
        )
        {
            std::string rawBytes =
                objectStore.retrieve(hash);


            if (
                !uploadToMinIO(
                    url,
                    rawBytes
                )
            )
            {
                std::cerr
                    << "[Push] Failed to upload object: "
                    << hash
                    << std::endl;


                curl_global_cleanup();

                return false;
            }


            std::cout
                << "[Push] Uploaded: "
                << hash.substr(0, 8)
                << "..."
                << std::endl;
        }
    }

    if (!finalizePush(serverUrl))
    {
        std::cerr
            << "[Push] Objects uploaded, but repository "
               "state could not be finalized."
            << std::endl;


        curl_global_cleanup();

        return false;
    }


    curl_global_cleanup();


    std::cout
        << "[Push] Push completed successfully."
        << std::endl;


    return true;
}

}