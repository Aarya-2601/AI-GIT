#include "clone.hpp"
#include "../storage/metadata_db.hpp"
#include "../storage/object_store.hpp"
#include "../core/filesystem.hpp"

#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <map>

namespace fs=std::filesystem;

namespace Commands{
//st:1 incoming network bytes from Express appended into std::string
static size_t expressBytesClone(void* contents, size_t size, size_t nmemb, std::string* outputString){
    size_t totalBytes=size*nmemb;
    outputString->append(static_cast<char*>(contents), totalBytes);
    return totalBytes;
}

//writes incoming binary stream bytes from MinIO to a file on disk
static size_t minioWriteClone(void* contents, size_t size, size_t nmemb, std::ofstream* fileStream){
    size_t totalBytes=size*nmemb;
    fileStream->write(static_cast<char*>(contents), totalBytes);
    return totalBytes;
}

//st 2: sends GET request to backend for manifest JSON
static std::string getManifestClone(const std::string& serverUrl, const std::string& repoName){
    std::string cloneEndpoint=serverUrl+"/api/v1/pull/clone/"+repoName;
    std::string jsonResponse;

    CURL* curl=curl_easy_init();
    if (!curl) return "";

    curl_easy_setopt(curl, CURLOPT_URL, cloneEndpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handleStringResponse);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonResponse);

    CURLcode res=curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res!=CURLE_OK) {
        std::cerr<<"[Clone] Network Error: Failed to contact backend at "<<cloneEndpoint<<std::endl;
        return "";
    }

    return jsonResponse;
}

//hash:url map
static std::map<std::string, std::string> createMapClone(const std::string& json){
    std::map<std::string, std::string> urlMap;
    size_t pos=0;

    while((pos=json.find("\"", pos))!=std::string::npos){
        size_t keyStart=pos+1;
        size_t keyEnd=json.find("\"", keyStart);
        if(keyEnd==std::string::npos) break;

        std::string key=json.substr(keyStart, keyEnd-keyStart);
        
        size_t urlStart=json.find("\"http", keyEnd);
        if(urlStart==std::string::npos) break;

        size_t urlEnd=json.find("\"", urlStart+1);
        if(urlEnd==std::string::npos) break;

        std::string url=json.substr(urlStart+1, urlEnd-urlStart-1);
        urlMap[key]=url;

        pos=urlEnd+1;
    }

    return urlMap;
}

//st:3 connects to MinIO using libcurl GET and writes to disk
static bool downloadChunkClone(const std::string& url, const fs::path& objectPath) {
    fs::create_directories(objectPath.parent_path());

    std::ofstream fileOnDisk(objectPath, std::ios::binary);
    if (!fileOnDisk.is_open()) return false;

    CURL* curl=curl_easy_init();
    if (!curl){
        fileOnDisk.close();
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handleFileWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fileOnDisk);

    CURLcode result=curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fileOnDisk.close();

    return(result==CURLE_OK);
}

bool runClone(const std::string& reponame, const std::string server){
    metadataDB.setConfig("remote_repo", reponame);
    curl_global_init(CURL_GLOBAL_ALL);

    std::cout<<"[Clone] Setting up local .aigit directory for '"<<reponame<<"'..."<<std::endl;

    Storage::ObjectStore objectStore(".aigit");
    objectStore.initialize();
    Storage::MetadataDB metadataDB(".aigit/metadata.db");

    std::string jsonResponse=getManifestClone(server, reponame);
    if(jsonResponse.empty()){
        curl_global_cleanup();
        return false;
    }

    std::map<std::string, std::string> downloadMap=parseManifestJson(jsonResponse);
    if(downloadMap.empty()){
        std::cerr<<"[Clone] Error: No chunks returned or repository '"<<reponame<<"' not found."<<std::endl;
        curl_global_cleanup();
        return false;
    }

    std::cout<<"[Clone] Downloading "<<downloadMap.size()<<" chunk(s) from MinIO..."<<std::endl;

    size_t successCount=0;
    for(const auto& [hash, url]:downloadMap){
        fs::path objectPath=fs::path(".aigit")/"objects"/hash.substr(0, 2)/hash.substr(2);

        if(downloadChunkClone(url, objectPath)){
            long long fileSize = static_cast<long long>(fs::file_size(objectPath));
            metadataDB.addObject(hash, fileSize, "chunk");
            
            std::cout<<"Downloaded chunk: "<<hash.substr(0, 8)<<"..."<<std::endl;
            successCount++;
        } 
        else{
            std::cerr<<"Failed to download chunk: "<<hash.substr(0, 8)<<"..."<<std::endl;
        }
    }

    std::cout<<[Clone] Complete! ("<<successCount<<"/"<<downloadMap.size()<<" chunks saved)"<<std::endl;

    curl_global_cleanup();
    return successCount==downloadMap.size();
}
}