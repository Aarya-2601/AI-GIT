#pragma once

// Shared libcurl CURLOPT_WRITEFUNCTION callbacks.
//
// push.cpp / pull.cpp / clone.cpp all need to (a) collect an HTTP response
// body into a std::string, and (b) stream a binary HTTP response straight
// to a file on disk. Previously each of pull.cpp / clone.cpp declared its
// own local static versions of these (expressBytesPull/expressBytesClone,
// minioWritePull/minioWriteClone) AND ALSO called two more names
// (handleStringResponse / handleFileWrite) that were never defined anywhere
// -- that mismatch is why the project failed to compile. Centralizing them
// here removes the duplication and the undefined-symbol bug in one place.

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

namespace Utils
{
    // Appends the received bytes onto *out. Pass a std::string* as the
    // CURLOPT_WRITEDATA payload.
    std::size_t curlWriteToString(
        void* contents,
        std::size_t size,
        std::size_t nmemb,
        std::string* out
    );

    // Writes the received bytes straight through to *out. Pass a
    // std::ofstream* (opened std::ios::binary) as CURLOPT_WRITEDATA.
    std::size_t curlWriteToFile(
        void* contents,
        std::size_t size,
        std::size_t nmemb,
        std::ofstream* out
    );

    // Shared remote-protocol helpers used by push/pull/clone. Consolidates
    // what used to be near-identical copies of these functions in
    // push.cpp / pull.cpp / clone.cpp.

    // Sends a GET request to `url`. Returns the raw response body, or ""
    // if the request itself failed (network/server error); logs with the
    // given [logTag] prefix on failure.
    std::string httpGet(const std::string& url, const std::string& logTag);

    // Sends a POST request with `jsonBody` as the payload. Returns the raw
    // response body, or "" if the request itself failed.
    std::string httpPostJson(
        const std::string& url,
        const std::string& jsonBody,
        const std::string& logTag
    );

    // Parses a {"status":"ok", <key>: {hash: url, ...}} response body into
    // a hash->url map. Returns an empty map on malformed JSON or a
    // missing/empty field.
    std::map<std::string, std::string> parseHashUrlMap(
        const std::string& json,
        const std::string& key,
        const std::string& logTag
    );

    // Streams one object from a presigned URL straight to its CAS path on
    // disk, creating parent directories as needed.
    bool downloadObjectToPath(
        const std::string& url,
        const std::filesystem::path& objectPath
    );
}