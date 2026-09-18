#include "curl_helpers.hpp"

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

}