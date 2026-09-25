#include "pull.hpp"
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

static std::string getRemoteRepository(
    const std::string& serverUrl,
    const std::string& repoName
)
{
    // Current backend contract used by clone as well.
    const std::string endpoint =serverUrl + "/api/v1/clone/" + repoName;
    return Utils::httpGet(endpoint, "Pull");
}

static std::string branchNameFromHead(
    const std::string& headRef
)
{
    const std::string prefix = "refs/heads/";

    if (headRef.rfind(prefix, 0) == 0)
    {
        return headRef.substr(prefix.size());
    }

    return "";
}

static bool writeTextFile(
    const fs::path& path,
    const std::string& contents
)
{
    try
    {
        if (path.has_parent_path())
        {
            fs::create_directories(path.parent_path());
        }

        std::ofstream out(path, std::ios::binary);

        if (!out)
        {
            return false;
        }

        out << contents;

        return static_cast<bool>(out);
    }
    catch (...)
    {
        return false;
    }
}

bool runPull(
    const std::string& reponame,
    const std::string& server
)
{
    if (!fs::exists(".aigit"))
    {
        std::cerr
            << "[Pull] Error: Not an AI-Git repository."
            << std::endl;

        return false;
    }

    // --------------------------------------------------------
    // 1. Determine which remote repository this clone tracks.
    // --------------------------------------------------------

    Core::Config config;
    config.load(".aigit/config");

    std::string repoToPull = reponame;

    if (repoToPull.empty() || repoToPull == "default-repo")
    {
        repoToPull = config.get("remote.repo");
    }

    if (repoToPull.empty())
    {
        std::cerr
            << "[Pull] Error: Could not determine remote repository."
            << std::endl;

        return false;
    }

    std::string serverUrl = server;

    if (serverUrl.empty())
    {
        serverUrl = config.get(
            "remote.server",
            "http://localhost:3000"
        );
    }

    std::cout
        << "[Pull] Checking remote repository '"
        << repoToPull
        << "' for updates..."
        << std::endl;

    curl_global_init(CURL_GLOBAL_ALL);

    // --------------------------------------------------------
    // 2. Ask backend for repository metadata + download URLs.
    // --------------------------------------------------------

    const std::string jsonResponse =
        getRemoteRepository(serverUrl, repoToPull);

    if (jsonResponse.empty())
    {
        curl_global_cleanup();
        return false;
    }

    nlohmann::json response;

    try
    {
        response = nlohmann::json::parse(jsonResponse);
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "[Pull] Error: Server returned malformed JSON: "
            << e.what()
            << std::endl;

        curl_global_cleanup();
        return false;
    }

    if (
        !response.contains("status") ||
        response["status"] != "ok" ||
        !response.contains("repository") ||
        !response["repository"].is_object() ||
        !response.contains("download_urls") ||
        !response["download_urls"].is_object()
    )
    {
        std::cerr
            << "[Pull] Error: Invalid response from backend."
            << std::endl;

        curl_global_cleanup();
        return false;
    }

    const nlohmann::json& repository =
        response["repository"];

    if (
        !repository.contains("head") ||
        !repository["head"].is_string() ||
        !repository.contains("refs") ||
        !repository["refs"].is_object()
    )
    {
        std::cerr
            << "[Pull] Error: Remote repository metadata is incomplete."
            << std::endl;

        curl_global_cleanup();
        return false;
    }

    const std::string remoteHead =repository["head"].get<std::string>();

    const std::string branchName =branchNameFromHead(remoteHead);

    if (branchName.empty())
    {
        std::cerr
            << "[Pull] Error: Unsupported remote HEAD: "
            << remoteHead
            << std::endl;

        curl_global_cleanup();
        return false;
    }

    const nlohmann::json& remoteRefs =
        repository["refs"];

    if (
        !remoteRefs.contains(remoteHead) ||
        !remoteRefs[remoteHead].is_string()
    )
    {
        std::cerr
            << "[Pull] Error: Remote HEAD does not point to a commit."
            << std::endl;

        curl_global_cleanup();
        return false;
    }

    const std::string remoteCommit =
        remoteRefs[remoteHead].get<std::string>();

    // --------------------------------------------------------
    // 3. Parse remote object download URLs.
    // --------------------------------------------------------

    std::map<std::string, std::string> downloadUrls;

    try
    {
        for (
            auto it = response["download_urls"].begin();
            it != response["download_urls"].end();
            ++it
        )
        {
            if (it.value().is_string())
            {
                downloadUrls[it.key()] =
                    it.value().get<std::string>();
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "[Pull] Error: Could not parse download URLs: "
            << e.what()
            << std::endl;

        curl_global_cleanup();
        return false;
    }

    // --------------------------------------------------------
    // 4. Find which remote objects are missing locally.
    // --------------------------------------------------------

    Storage::MetadataDB metadataDB(
        ".aigit/metadata.db"
    );

    Storage::ObjectStore objectStore(
        ".aigit"
    );

    std::map<std::string, std::string> missingObjects;

    for (const auto& [hash, url] : downloadUrls)
    {
        if (!metadataDB.objectExists(hash))
        {
            missingObjects[hash] = url;
        }
    }

    std::cout
        << "[Pull] Remote references "
        << downloadUrls.size()
        << " object(s); "
        << missingObjects.size()
        << " missing locally."
        << std::endl;

    // --------------------------------------------------------
    // 5. Download only missing objects.
    // --------------------------------------------------------

    std::size_t downloaded = 0;

    for (const auto& [hash, url] : missingObjects)
    {
        std::string data;

        if (!Utils::downloadObjectToString(url, data))
        {
            std::cerr
                << "[Pull] Error downloading "
                << hash.substr(0, 8)
                << "..."
                << std::endl;

            curl_global_cleanup();
            return false;
        }

        const std::string actualHash =
            Core::calcSHA256(data);

        if (actualHash != hash)
        {
            std::cerr
                << "[Pull] Hash verification failed for "
                << hash.substr(0, 8)
                << "..."
                << std::endl;

            curl_global_cleanup();
            return false;
        }

        // "unknown" is intentional here: the downloaded object already
        // contains its serialized AI-Git object representation. This is
        // the same approach used by the working clone path.
        objectStore.storeObject(
            hash,
            data,
            "unknown"
        );

        metadataDB.addObject(
            hash,
            static_cast<long long>(data.size()),
            "unknown"
        );

        ++downloaded;

        std::cout
            << "[Pull] Downloaded "
            << downloaded
            << "/"
            << missingObjects.size()
            << "  "
            << hash.substr(0, 8)
            << "..."
            << std::endl;
    }

    // --------------------------------------------------------
    // 6. Update local refs to match the remote repository.
    // --------------------------------------------------------

    for (
        auto it = remoteRefs.begin();
        it != remoteRefs.end();
        ++it
    )
    {
        if (!it.value().is_string())
        {
            continue;
        }

        const std::string refName =
            it.key();

        const std::string commitHash =
            it.value().get<std::string>();

        const fs::path refPath =
            fs::path(".aigit") / fs::path(refName);

        if (!writeTextFile(refPath, commitHash + "\n"))
        {
            std::cerr
                << "[Pull] Error: Could not update ref "
                << refName
                << std::endl;

            curl_global_cleanup();
            return false;
        }
    }

    // --------------------------------------------------------
    // 7. Checkout the updated branch.
    //
    // Important:
    // checkout needs to know the CURRENT commit so it can remove the
    // currently tracked files before restoring the new snapshot.
    //
    // We therefore temporarily keep HEAD on a bootstrap ref pointing to
    // the old local commit, then checkout the newly-updated branch.
    // --------------------------------------------------------

    std::string oldHeadContents;
    std::string oldCommit;

    {
        std::ifstream headIn(
            ".aigit/HEAD",
            std::ios::binary
        );

        if (headIn)
        {
            std::getline(headIn, oldHeadContents);
        }
    }

    if (
        oldHeadContents.rfind("ref: ", 0) == 0
    )
    {
        const std::string oldRef =
            oldHeadContents.substr(5);

        std::ifstream oldRefIn(
            fs::path(".aigit") / fs::path(oldRef),
            std::ios::binary
        );

        if (oldRefIn)
        {
            std::getline(oldRefIn, oldCommit);
        }
    }

    const fs::path bootstrapRef =
        ".aigit/refs/heads/__pull_bootstrap__";

    if (!oldCommit.empty())
    {
        if (
            !writeTextFile(
                bootstrapRef,
                oldCommit + "\n"
            ) ||
            !writeTextFile(
                ".aigit/HEAD",
                "ref: refs/heads/__pull_bootstrap__\n"
            )
        )
        {
            std::cerr
                << "[Pull] Error: Could not prepare checkout."
                << std::endl;

            curl_global_cleanup();
            return false;
        }
    }

    const int checkoutResult =
        runCheckout(branchName);

    std::error_code ec;
    fs::remove(bootstrapRef, ec);

    if (checkoutResult != 0)
    {
        std::cerr
            << "[Pull] Error: Download succeeded but checkout failed."
            << std::endl;

        curl_global_cleanup();
        return false;
    }

    // --------------------------------------------------------
    // 8. Persist remote tracking information.
    // --------------------------------------------------------

    config.set(
        "remote.repo",
        repoToPull
    );

    config.set(
        "remote.server",
        serverUrl
    );

    config.save(
        ".aigit/config"
    );

    std::cout
        << "[Pull] Pull completed successfully."
        << std::endl;

    std::cout
        << "[Pull] Updated "
        << branchName
        << " to "
        << remoteCommit.substr(0, 8)
        << "..."
        << std::endl;

    std::cout
        << "[Pull] Downloaded and verified "
        << downloaded
        << " new object(s)."
        << std::endl;

    curl_global_cleanup();
    return true;
}

}