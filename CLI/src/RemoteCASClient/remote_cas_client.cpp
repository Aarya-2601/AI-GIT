#include "remote_cas_client.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace Remote
{

namespace
{

size_t writeCallback(
    char* data,
    size_t size,
    size_t count,
    void* userData
)
{
    auto* response =
        static_cast<std::string*>(userData);

    response->append(
        data,
        size * count
    );

    return size * count;
}


std::string performRequest(
    const std::string& url,
    const std::string* jsonBody = nullptr
)
{
    CURL* curl = curl_easy_init();

    if (!curl)
    {
        throw std::runtime_error(
            "Could not initialize HTTP client."
        );
    }

    std::string responseBody;

    curl_slist* headers = nullptr;

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str()
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEFUNCTION,
        writeCallback
    );

    curl_easy_setopt(
        curl,
        CURLOPT_WRITEDATA,
        &responseBody
    );


    // If jsonBody exists,
    // this is a POST request.
    if (jsonBody != nullptr)
    {
        headers = curl_slist_append(
            headers,
            "Content-Type: application/json"
        );

        curl_easy_setopt(
            curl,
            CURLOPT_HTTPHEADER,
            headers
        );

        curl_easy_setopt(
            curl,
            CURLOPT_POST,
            1L
        );

        curl_easy_setopt(
            curl,
            CURLOPT_POSTFIELDS,
            jsonBody->c_str()
        );
    }


    CURLcode result =
        curl_easy_perform(curl);


    if (result != CURLE_OK)
    {
        std::string error =
            curl_easy_strerror(result);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        throw std::runtime_error(
            "HTTP request failed: " + error
        );
    }


    long statusCode = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &statusCode
    );


    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);


    if (statusCode < 200 ||
        statusCode >= 300)
    {
        throw std::runtime_error(
            "Remote server returned HTTP " +
            std::to_string(statusCode) +
            ": " +
            responseBody
        );
    }


    return responseBody;
}

}


RemoteCASClient::RemoteCASClient(
    const std::string& baseUrl
)
    : baseUrl(baseUrl)
{
}


bool RemoteCASClient::healthCheck() const
{
    try
    {
        performRequest(
            baseUrl + "/health"
        );

        return true;
    }
    catch (...)
    {
        return false;
    }
}


UploadNegotiation
RemoteCASClient::negotiateUpload(
    const std::vector<std::string>& objectIds
) const
{
    // Build:
    //
    // {
    //     "chunks": [
    //         "hash1",
    //         "hash2"
    //     ]
    // }

    nlohmann::json requestBody;

    requestBody["chunks"] =
        objectIds;


    std::string serializedBody =
        requestBody.dump();


    std::string responseBody =
        performRequest(
            baseUrl +
            "/api/v1/push/negotiate",

            &serializedBody
        );


    nlohmann::json response =
        nlohmann::json::parse(
            responseBody
        );


    UploadNegotiation result;


    if (response.contains(
            "upload_urls"))
    {
        for (
            auto& [objectId, url] :
            response["upload_urls"].items()
        )
        {
            result.uploadUrls[objectId] =
                url.get<std::string>();
        }
    }


    return result;
}

}