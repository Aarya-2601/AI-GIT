#include "clone.hpp"
#include "checkout.hpp"

#include "../storage/metadata_db.hpp"
#include "../storage/object_store.hpp"
#include "../core/config.hpp"
#include "../core/hashing.hpp"
#include "../helpers/curl_helpers.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

namespace fs = std::filesystem;

namespace Commands
{

// ------------------------------------------------------------
// Request the remote clone manifest.
//
// Backend returns:
//
// {
//   "repository": {
//      "name": "...",
//      "head": "refs/heads/main",
//      "refs": {...}
//   },
//   "download_urls": {
//      "<hash>": "<presigned GET URL>"
//   }
// }
// ------------------------------------------------------------

static std::string getCloneManifest(
    const std::string& serverUrl,
    const std::string& repoName
)
{
    const std::string endpoint =
        serverUrl +
        "/api/v1/clone/" +
        repoName;

    return Utils::httpGet(
        endpoint,
        "Clone"
    );
}


// ------------------------------------------------------------
// Write a text file, creating its parent directory first.
// ------------------------------------------------------------

static bool writeTextFile(
    const fs::path& path,
    const std::string& value
)
{
    try
    {
        if (path.has_parent_path())
        {
            fs::create_directories(
                path.parent_path()
            );
        }

        std::ofstream file(
            path,
            std::ios::binary |
            std::ios::trunc
        );

        if (!file)
        {
            return false;
        }

        file << value;

        return static_cast<bool>(file);
    }
    catch (...)
    {
        return false;
    }
}


// ------------------------------------------------------------
// Convert:
//
// refs/heads/main
//
// into:
//
// main
//
// runCheckout() accepts a branch name rather than a full ref.
// ------------------------------------------------------------

static std::string branchNameFromHead(
    const std::string& head
)
{
    const std::string prefix =
        "refs/heads/";

    if (head.rfind(prefix, 0) != 0)
    {
        return "";
    }

    return head.substr(
        prefix.size()
    );
}


// ------------------------------------------------------------
// Remove an incomplete clone if anything fails.
//
// Since clone refuses to run when .aigit already exists,
// leaving a half-created .aigit behind would otherwise make the
// next attempt unnecessarily awkward.
// ------------------------------------------------------------

static void cleanupFailedClone()
{
    std::error_code ec;

    fs::remove_all(
        ".aigit",
        ec
    );
}


// ------------------------------------------------------------
// Main clone command
// ------------------------------------------------------------

bool runClone(
    const std::string& repoName,
    const std::string& server
)
{
    if (fs::exists(".aigit"))
    {
        std::cerr
            << "[Clone] Error: '.aigit' already exists "
               "in this directory."
            << std::endl;

        return false;
    }


    curl_global_init(
        CURL_GLOBAL_ALL
    );


    // --------------------------------------------------------
    // 1. Ask backend for repository state + object URLs
    // --------------------------------------------------------

    const std::string response =
        getCloneManifest(
            server,
            repoName
        );


    if (response.empty())
    {
        std::cerr
            << "[Clone] Could not retrieve remote repository."
            << std::endl;

        curl_global_cleanup();

        return false;
    }


    nlohmann::json manifest;


    try
    {
        manifest =
            nlohmann::json::parse(
                response
            );
    }
    catch (
        const std::exception& e
    )
    {
        std::cerr
            << "[Clone] Invalid backend response: "
            << e.what()
            << std::endl;

        curl_global_cleanup();

        return false;
    }


    if (
        !manifest.contains("repository") ||
        !manifest["repository"].is_object() ||
        !manifest.contains("download_urls") ||
        !manifest["download_urls"].is_object()
    )
    {
        std::cerr
            << "[Clone] Backend response is missing "
               "repository metadata or download URLs."
            << std::endl;

        curl_global_cleanup();

        return false;
    }


    const nlohmann::json repository =
        manifest["repository"];


    if (
        !repository.contains("head") ||
        !repository["head"].is_string() ||
        !repository.contains("refs") ||
        !repository["refs"].is_object()
    )
    {
        std::cerr
            << "[Clone] Remote repository metadata is invalid."
            << std::endl;

        curl_global_cleanup();

        return false;
    }


    const std::string headRef =
        repository["head"].get<std::string>();


    const nlohmann::json refs =
        repository["refs"];


    if (
        !refs.contains(headRef) ||
        !refs[headRef].is_string()
    )
    {
        std::cerr
            << "[Clone] Remote HEAD does not point "
               "to a valid branch."
            << std::endl;

        curl_global_cleanup();

        return false;
    }


    const std::string branchName =
        branchNameFromHead(
            headRef
        );


    if (branchName.empty())
    {
        std::cerr
            << "[Clone] Detached/non-branch HEAD is not "
               "supported by this prototype."
            << std::endl;

        curl_global_cleanup();

        return false;
    }


    const std::map<std::string, std::string>
        downloadMap =
            Utils::parseHashUrlMap(
                response,
                "download_urls",
                "Clone"
            );


    if (downloadMap.empty())
    {
        std::cerr
            << "[Clone] Remote repository contains "
               "no downloadable objects."
            << std::endl;

        curl_global_cleanup();

        return false;
    }


    std::cout
        << "[Clone] Repository: "
        << repoName
        << std::endl;

    std::cout
        << "[Clone] Objects to download: "
        << downloadMap.size()
        << std::endl;


    // --------------------------------------------------------
    // 2. Create local AI-Git storage
    // --------------------------------------------------------

    try
    {
        fs::create_directories(
            ".aigit/refs/heads"
        );


        Storage::ObjectStore objectStore(
            ".aigit"
        );

        objectStore.initialize();


        Storage::MetadataDB metadataDB(
            ".aigit/metadata.db"
        );

        metadataDB.initialize();


        // ----------------------------------------------------
        // 3. Download every remote object
        // ----------------------------------------------------

        std::size_t downloaded = 0;


        for (
            const auto& [hash, url] :
            downloadMap
        )
        {
            std::string data;


            if (
                !Utils::downloadObjectToString(
                    url,
                    data
                )
            )
            {
                std::cerr
                    << "[Clone] Failed to download object "
                    << hash
                    << std::endl;

                cleanupFailedClone();

                curl_global_cleanup();

                return false;
            }


            // Never trust downloaded bytes merely because MinIO
            // returned HTTP 200.
            const std::string actualHash =
                Core::calcSHA256(
                    data
                );


            if (actualHash != hash)
            {
                std::cerr
                    << "[Clone] Integrity check failed for object "
                    << hash
                    << std::endl;

                cleanupFailedClone();

                curl_global_cleanup();

                return false;
            }


            /*
             * The object ID is based on the logical/raw object bytes.
             *
             * storeObject() writes those bytes into AI-Git's normal
             * local object format, including its header/compression.
             *
             * We initially use "unknown" because clone does not need
             * to duplicate object-graph parsing merely to decide a
             * metadata label.
             */
            objectStore.storeObject(
                hash,
                data,
                "unknown"
            );


            metadataDB.addObject(
                hash,
                static_cast<long long>(
                    data.size()
                ),
                "unknown"
            );


            ++downloaded;


            std::cout
                << "[Clone] Downloaded "
                << downloaded
                << "/"
                << downloadMap.size()
                << "  "
                << hash.substr(0, 8)
                << "..."
                << std::endl;
        }


        // ----------------------------------------------------
        // 4. Restore repository refs
        // ----------------------------------------------------

        for (
            auto it = refs.begin();
            it != refs.end();
            ++it
        )
        {
            if (!it.value().is_string())
            {
                std::cerr
                    << "[Clone] Invalid remote ref: "
                    << it.key()
                    << std::endl;

                cleanupFailedClone();

                curl_global_cleanup();

                return false;
            }


            const std::string refName =
                it.key();


            /*
             * For the current standalone VCS prototype we accept
             * branch refs only.
             */
            const std::string refPrefix =
                "refs/heads/";


            if (
                refName.rfind(
                    refPrefix,
                    0
                ) != 0
            )
            {
                continue;
            }


            const fs::path refPath =
                fs::path(".aigit") /
                fs::path(refName);


            if (
                !writeTextFile(
                    refPath,
                    it.value().get<std::string>() +
                    "\n"
                )
            )
            {
                std::cerr
                    << "[Clone] Failed to create ref "
                    << refName
                    << std::endl;

                cleanupFailedClone();

                curl_global_cleanup();

                return false;
            }
        }


        // ----------------------------------------------------
        // 5. Create HEAD
        //
        // Temporarily leave HEAD without a current commit.
        //
        // runCheckout(branchName) will restore the downloaded
        // snapshot and then set HEAD to the correct branch.
        //
        // This is intentional: if HEAD already pointed at the
        // target commit before checkout, checkout would consider
        // those files "currently tracked" and try to remove them.
        // ----------------------------------------------------

        if (
            !writeTextFile(
                ".aigit/HEAD",
                "ref: refs/heads/__clone_bootstrap__\n"
            )
        )
        {
            std::cerr
                << "[Clone] Failed to create HEAD."
                << std::endl;

            cleanupFailedClone();

            curl_global_cleanup();

            return false;
        }


        // ----------------------------------------------------
        // 6. Create empty index
        //
        // runCheckout() will rebuild it from the target tree.
        // ----------------------------------------------------

        {
            std::ofstream index(
                ".aigit/index",
                std::ios::trunc
            );


            if (!index)
            {
                std::cerr
                    << "[Clone] Failed to create index."
                    << std::endl;

                cleanupFailedClone();

                curl_global_cleanup();

                return false;
            }
        }


        // ----------------------------------------------------
        // 7. Save remote identity
        // ----------------------------------------------------

        Core::Config config;

        config.set(
            "core.repositoryformatversion",
            "0"
        );

        config.set(
            "remote.repo",
            repoName
        );

        config.set(
            "remote.server",
            server
        );

        config.save(
            ".aigit/config"
        );


        // ----------------------------------------------------
        // 8. Restore working directory using EXISTING checkout
        //
        // checkout already knows:
        //
        // commit -> tree -> files
        //                  -> manifest -> FastCDC chunks
        //
        // so clone must not implement reconstruction again.
        // ----------------------------------------------------

        const int checkoutResult =
            runCheckout(
                branchName
            );


        if (checkoutResult != 0)
        {
            std::cerr
                << "[Clone] Objects downloaded, but working "
                   "directory reconstruction failed."
                << std::endl;

            cleanupFailedClone();

            curl_global_cleanup();

            return false;
        }


        // ----------------------------------------------------
        // 9. Remove bootstrap ref if one was ever materialized.
        // ----------------------------------------------------

        {
            std::error_code ec;

            fs::remove(
                ".aigit/refs/heads/__clone_bootstrap__",
                ec
            );
        }


        std::cout
            << "[Clone] Clone completed successfully."
            << std::endl;

        std::cout
            << "[Clone] Restored branch: "
            << branchName
            << std::endl;

        std::cout
            << "[Clone] Downloaded and verified "
            << downloaded
            << " object(s)."
            << std::endl;
    }
    catch (
        const std::exception& e
    )
    {
        std::cerr
            << "[Clone] Error: "
            << e.what()
            << std::endl;

        cleanupFailedClone();

        curl_global_cleanup();

        return false;
    }


    curl_global_cleanup();

    return true;
}

}