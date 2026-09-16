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
#include <fstream>
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
}