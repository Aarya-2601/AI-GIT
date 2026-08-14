    //read all local hashes
//send a POST request with the chunks we have
//request reaches postgreSQL and it check the missing or modified chunks
//returns a presigned MinIo url for the missing chunks
//read the raw bytes of missing chunks 
//send an HTTP PUT request to minIO using libcurl

#include "push.hpp"
#include "../storage/metadata_db.hpp"
#include "../storage/object_store.hpp"

#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

namespace Commands{
    //st 1
    std::vector<std::string> getLocalHashes(const Storage::MetadataDB& db){
        return db.getAllObjectIds();
    }

    //st 2
    std::string talkWithBackend(const std::string& server, std::vector<std::string>& hashes){
        std::string jsonPaylod="{\"chunks\": [";
        for (size_t i=0; i<hashes.size(); ++i){
            jsonPayload+="\""+hashes[i]+"\"";
            if(i+1 < hashes.size()) jsonPayload+=", ";
        }
        jsonPayload+="]}";

        CURL* curl=curl_easy_init();
        std::string response;
        if (!curl) return response;

        struct curl_slist* headers=nullptr;
        headers=curl_slist_append(headers, "Content-Type: application/json");

        std::string endpoint=serverUrl+"/api/v1/push/negotiate";
        curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* ptr, size_t size, size_t nmemb, std::string* data) -> size_t {
            data->append(static_cast<char*>(ptr), size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return response;  //st 3 and st 4
    }

    //st 5 and st 6
    bool uploadtoMinio(const std::string& presignedUrl, const std::string& rawBytes){
        CURL* curl=curl_easy_init();
        if (!curl) return false;

        struct curl_slist* headers=nullptr;
        headers=curl_slist_append(headers, "Content-Type: application/octet-stream");

        curl_easy_setopt(curl, CURLOPT_URL, presignedUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, rawBytes.data());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(rawBytes.size()));

        CURLcode res=curl_easy_perform(curl);
        long responseCode=0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return (res==CURLE_OK && responseCode>200 && responseCode<300);
    }

    bool runPush(const std::string& serverUrl){
        curl_global_init(CURL_GLOBAL_ALL);

        Storage::MetadataDB metadataDB(".aigit/metadata.db");
        Storage::ObjectStore objectStore(".aigit");

        std::vector<std::string> hashes=getLocalHashes(metadataDB);
        if (hashes.empty()) return true;

        std::string responseJson=talkWithBackend(serverUrl, hashes);

        //parse response URLs into a map, hash to presignedUrl
        std::map<std::string, std::string> missingChunks=extractUrlsFromJson(responseJson);

        for(const auto& [hash, url]:missingChunks){
            std::string rawBytes=objectStore.retrieve(hash);
            uploadToMinIO(url, rawBytes);
        }

        curl_global_cleanup();
        return true;
    }
}