// AI-Git remote push.
//
// Flow:
// 1. Read all locally tracked CAS/VCS object IDs from MetadataDB.
// 2. Ask the backend which objects are missing remotely.
// 3. Upload only those missing objects.
// 4. Read local HEAD + branch refs.
// 5. Finalize the push by publishing repository state and the
//    repository object catalogue.

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


// ------------------------------------------------------------
// Local object catalogue
// ------------------------------------------------------------

static std::vector<std::string> getLocalHashes(
    const Storage::MetadataDB& db
)
{
    return db.getAllObjectIds();
}


// ------------------------------------------------------------
// Small text-file helper
// ------------------------------------------------------------

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

    std::getline(
        file,
        value
    );

    while (
        !value.empty() &&
        (
            value.back() == '\r' ||
            value.back() == '\n'
        )
    )
    {
        value.pop_back();
    }

    return value;
}


// ------------------------------------------------------------
// Repository identity
//
// For the current prototype, the remote repository name is the
// current working-directory name.
// ------------------------------------------------------------

static std::string getRepositoryName()
{
    return fs::current_path()
        .filename()
        .string();
}


// ------------------------------------------------------------
// Read symbolic HEAD.
//
// .aigit/HEAD:
//
//     ref: refs/heads/main
//
// returns:
//
//     refs/heads/main
// ------------------------------------------------------------

static std::string getHeadRef()
{
    const std::string head =
        readTextFile(".aigit/HEAD");

    const std::string prefix =
        "ref: ";

    if (head.rfind(prefix, 0) != 0)
    {
        return "";
    }

    return head.substr(
        prefix.size()
    );
}


// ------------------------------------------------------------
// Read all local branches.
//
// Produces JSON such as:
//
// {
//     "refs/heads/main": "<commit>",
//     "refs/heads/dev":  "<commit>"
// }
// ------------------------------------------------------------

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
        fs::recursive_directory_iterator(
            headsDir
        )
    )
    {
        if (!entry.is_regular_file())
        {
            continue;
        }


        const fs::path relative =
            fs::relative(
                entry.path(),
                ".aigit"
            );


        const std::string refName =
            relative.generic_string();


        const std::string commitHash =
            readTextFile(
                entry.path()
            );


        if (!commitHash.empty())
        {
            refs[refName] =
                commitHash;
        }
    }


    return refs;
}


// ------------------------------------------------------------
// Phase 1: negotiate remote deduplication
// ------------------------------------------------------------

static std::string negotiatePush(
    const std::string& serverUrl,
    const std::vector<std::string>& hashes
)
{
    nlohmann::json payload;

    payload["chunks"] =
        hashes;


    const std::string endpoint =
        serverUrl +
        "/api/v1/push/negotiate";


    return Utils::httpPostJson(
        endpoint,
        payload.dump(),
        "Push"
    );
}


// ------------------------------------------------------------
// Upload one object through a MinIO presigned PUT URL.
//
// ObjectStore::retrieve() returns the logical/raw object bytes,
// even if the local on-disk representation is compressed.
// ------------------------------------------------------------

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
        CURLOPT_POSTFIELDSIZE_LARGE,
        static_cast<curl_off_t>(
            rawBytes.size()
        )
    );


    const CURLcode result =
        curl_easy_perform(curl);


    long responseCode = 0;


    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &responseCode
    );


    curl_slist_free_all(
        headers
    );

    curl_easy_cleanup(
        curl
    );


    return (
        result == CURLE_OK &&
        responseCode >= 200 &&
        responseCode < 300
    );
}


// ------------------------------------------------------------
// Phase 2: publish repository state.
//
// This happens AFTER every requested object upload succeeds.
// ------------------------------------------------------------

static bool finalizePush(
    const std::string& serverUrl,
    const std::vector<std::string>& hashes
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
            << "[Push] HEAD is invalid or detached."
            << std::endl;

        return false;
    }


    if (!refs.contains(headRef))
    {
        std::cerr
            << "[Push] HEAD points to a branch "
               "that has no commit."
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

    payload["objects"] =
        hashes;


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


// ------------------------------------------------------------
// Public push command
// ------------------------------------------------------------

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


    const std::vector<std::string> hashes =
        getLocalHashes(
            metadataDB
        );


    if (hashes.empty())
    {
        std::cout
            << "[Push] Nothing to push."
            << std::endl;


        curl_global_cleanup();

        return true;
    }


    const std::string responseJson =
        negotiatePush(
            serverUrl,
            hashes
        );


    if (responseJson.empty())
    {
        curl_global_cleanup();

        return false;
    }


    const std::map<std::string, std::string>
        uploadUrls =
            Utils::parseHashUrlMap(
                responseJson,
                "upload_urls",
                "Push"
            );


    if (uploadUrls.empty())
    {
        std::cout
            << "[Push] Remote already has all "
            << hashes.size()
            << " object(s)."
            << std::endl;
    }
    else
    {
        std::cout
            << "[Push] Remote is missing "
            << uploadUrls.size()
            << " of "
            << hashes.size()
            << " object(s)."
            << std::endl;


        for (
            const auto& [hash, url] :
            uploadUrls
        )
        {
            std::string rawBytes;


            try
            {
                rawBytes =
                    objectStore.retrieve(
                        hash
                    );
            }
            catch (
                const std::exception& e
            )
            {
                std::cerr
                    << "[Push] Could not read local object "
                    << hash
                    << ": "
                    << e.what()
                    << std::endl;


                curl_global_cleanup();

                return false;
            }


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
                << "[Push] Uploaded "
                << hash.substr(0, 8)
                << "..."
                << std::endl;
        }
    }


    /*
     * Do NOT return early when uploadUrls is empty.
     *
     * MinIO may already contain every object while this repository's
     * HEAD has changed. Repository state must therefore be finalized
     * independently of object deduplication.
     */

    if (
        !finalizePush(
            serverUrl,
            hashes
        )
    )
    {
        std::cerr
            << "[Push] Object transfer completed, "
               "but repository state could not be finalized."
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