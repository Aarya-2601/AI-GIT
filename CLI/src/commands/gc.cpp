#include "gc.hpp"
#include "../storage/object_store.hpp"
#include "../storage/metadata_db.hpp"
#include "../core/object_io.hpp"
#include "../core/index.hpp"
#include "../models/tree.hpp"
#include "../models/commit.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace Commands
{

namespace
{

// True as long as every object visited was readable/verifiable. Set to
// false the moment loadTree/loadCommit/retrieve fails anywhere in the
// walk -- gc then refuses to sweep at all, since an unreadable object
// might sit between a ref and something that's actually still reachable
// through it; better to do nothing than risk deleting live data.
bool g_markingComplete = true;

void markBlob(
    const std::string& blobHash,
    Storage::ObjectStore& objectStore,
    std::set<std::string>& reachable
)
{
    if (!reachable.insert(blobHash).second)
    {
        return; // already visited
    }

    std::string data;

    try
    {
        data = objectStore.retrieve(blobHash);
    }
    catch (const std::exception&)
    {
        g_markingComplete = false;
        return;
    }

    // A blob may actually be a chunk manifest (StorageManager::storeFile
    // stores large files this way) -- same JSON-shape check
    // StorageManager::restoreFile uses to tell the two apart.
    nlohmann::json manifest = nlohmann::json::parse(data, nullptr, false);

    if (
        !manifest.is_discarded() &&
        manifest.is_object() &&
        manifest.value("type", "") == "manifest" &&
        manifest.contains("chunks") &&
        manifest["chunks"].is_array()
    )
    {
        for (const auto& chunk : manifest["chunks"])
        {
            if (chunk.contains("hash") && chunk["hash"].is_string())
            {
                reachable.insert(chunk["hash"].get<std::string>());
            }
        }
    }
}

void markTree(
    const std::string& treeHash,
    Storage::ObjectStore& objectStore,
    std::set<std::string>& reachable
)
{
    if (!reachable.insert(treeHash).second)
    {
        return;
    }

    Models::Tree tree;

    try
    {
        tree = Core::loadTree(treeHash);
    }
    catch (const std::exception&)
    {
        g_markingComplete = false;
        return;
    }

    for (const auto& entry : tree.getEntries())
    {
        if (entry.isSubtree)
        {
            markTree(entry.hash, objectStore, reachable);
        }
        else
        {
            markBlob(entry.hash, objectStore, reachable);
        }
    }
}

void markCommit(
    const std::string& startCommitHash,
    Storage::ObjectStore& objectStore,
    std::set<std::string>& reachable
)
{
    std::string current = startCommitHash;

    // Walk first-parent history iteratively (bounded by recursion depth
    // only for extra merge parents, not by history length).
    while (!current.empty())
    {
        if (!reachable.insert(current).second)
        {
            return; // this commit (and everything behind it) is already marked
        }

        Models::Commit commit;

        try
        {
            commit = Core::loadCommit(current);
        }
        catch (const std::exception&)
        {
            g_markingComplete = false;
            return;
        }

        markTree(commit.getTreeHash(), objectStore, reachable);

        const std::vector<std::string>& parents = commit.getParentHashes();

        if (parents.empty())
        {
            return;
        }

        for (std::size_t i = 1; i < parents.size(); ++i)
        {
            markCommit(parents[i], objectStore, reachable);
        }

        current = parents[0];
    }
}

std::set<std::string> collectReachable(Storage::ObjectStore& objectStore)
{
    std::set<std::string> reachable;

    // Every branch ref, not just the current one -- gc must never delete
    // anything reachable from ANY ref, checked out or not. (This repo
    // has no detached-HEAD checkout and no stash/reflog, so refs/heads/*
    // plus the index below is the complete set of roots.)
    fs::path headsPath = ".aigit/refs/heads";

    if (fs::exists(headsPath))
    {
        for (const auto& entry : fs::directory_iterator(headsPath))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            std::ifstream refFile(entry.path());
            std::string commitHash;
            refFile >> commitHash;

            if (!commitHash.empty())
            {
                markCommit(commitHash, objectStore, reachable);
            }
        }
    }

    // Staged-but-uncommitted content: the index can point at blobs/
    // manifests no commit references yet.
    Core::Index index;
    index.load(".aigit/index");

    for (const auto& [path, entry] : index.getEntries())
    {
        markBlob(entry.hash, objectStore, reachable);
    }

    return reachable;
}

}

int runGc()
{
    if (!fs::exists(".aigit"))
    {
        std::cerr << "Error: Not an AI-Git repository." << std::endl;
        return 1;
    }

    g_markingComplete = true;

    Storage::ObjectStore objectStore(".aigit");
    Storage::MetadataDB metadataDB(".aigit/metadata.db");
    metadataDB.initialize();

    std::set<std::string> reachable = collectReachable(objectStore);

    if (!g_markingComplete)
    {
        std::cerr
            << "[gc] Error: could not fully walk reachable history "
            << "(a commit/tree/blob failed to read). Refusing to delete "
            << "anything -- run 'ai-git fsck' first to find the problem."
            << std::endl;
        return 1;
    }

    std::vector<std::string> corruptObjectIds;
    std::vector<Storage::ObjectStore::ObjectRecord> onDisk =
        objectStore.walkAll(&corruptObjectIds);

    if (!corruptObjectIds.empty())
    {
        std::cerr
            << "[gc] Error: " << corruptObjectIds.size()
            << " object(s) on disk failed verification. Refusing to "
            << "delete anything until this is resolved -- run "
            << "'ai-git fsck' for details." << std::endl;
        return 1;
    }

    std::size_t deleted = 0;

    for (const Storage::ObjectStore::ObjectRecord& record : onDisk)
    {
        if (reachable.count(record.objectId))
        {
            continue;
        }

        if (objectStore.remove(record.objectId))
        {
            metadataDB.removeObject(record.objectId);
            ++deleted;
        }
    }

    std::cout
        << "[gc] " << reachable.size() << " object(s) reachable, "
        << deleted << " unreachable object(s) removed." << std::endl;

    return 0;
}

}
