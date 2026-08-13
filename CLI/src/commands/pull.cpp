//send a GET request to get a list of chunks and presigned urls
////deduplication so comapre local with remote and discard those we already have
//download missing chunks from MinIo using libcurl
//update local db

#include "pull.hpp"
#include "../storage/metadata_db.hpp"
#include "../storage/object_store.hpp"

#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
namespace fs = std::filesystem;

namespace Commands{
//st:1 incoming network bytes from Express appended into std::string
static size_t expressBytesPull(void* contents, size_t size, size_t nmemb, std::string* outputString) {
    size_t totalBytes=size*nmemb;
    outputString->append(static_cast<char*>(contents), totalBytes);
    return totalBytes;
}

//writes incoming binary stream bytes from MinIO to a file on disk
static size_t minioWritePull(void* contents, size_t size, size_t nmemb, std::ofstream* fileStream) {
    size_t totalBytes=size*nmemb;
    fileStream->write(static_cast<char*>(contents), totalBytes);
    return totalBytes;
}
//st 2: sends GET request to backend for manifest JSON
static std::string getManifestPull(const std::string& serverUrl, const std::string& repoName) {
    std::string pullEndpoint=serverUrl+"/api/v1/pull/clone/"+repoName;
    std::string jsonResponse;

    CURL* curl=curl_easy_init();
    if(!curl) return "";

    curl_easy_setopt(curl, CURLOPT_URL, pullEndpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handleStringResponse);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonResponse);

    CURLcode res=curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if(res!=CURLE_OK){
        std::cerr<<"[Pull] Network Error: Could not reach server at "<<pullEndpoint<<std::endl;
        return "";
    }
    return jsonResponse;
}

//hash:url map
static std::map<std::string, std::string> createMapPull(const std::string& json){
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

//filter out chunks that already exist in local storage
static std::map<std::string, std::string> filterChunks(const std::map<std::string, std::string>& remoteManifest, const Storage::MetadataDB& metadataDB){
    std::map<std::string, std::string> missingChunks;

    for (const auto& [hash, url]:remoteManifest){
        //keep chunks not in local CAS
        if(!metadataDB.objectExists(hash)){
            missingChunks[hash]=url;
        }
    }
    return missingChunks;
}

//st:3 connects to MinIO using libcurl GET and writes to disk
static bool downloadChunkPull(const std::string& url, const fs::path& objectPath) {
    fs::create_directories(objectPath.parent_path());

    std::ofstream fileOnDisk(objectPath, std::ios::binary);
    if(!fileOnDisk.is_open()) return false;

    CURL* curl=curl_easy_init();
    if(!curl){
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


bool runPull(const std::string& reponame, const std::string server){
    std::string repoToPull=reponame;

    if(repoToPull.empty() || repoToPull=="default-repo"){
        repoToPull=metadataDB.getConfig("remote_repo");
        
        if(repoToPull.empty()){
            std::cerr<<"[Error] Could not determine remote repository name. Please specify it or clone the repo first."<<std::endl;
            return 1;
        }
    }
    
    curl_global_init(CURL_GLOBAL_ALL);

    std::cout<<"[Pull] Checking remote repository '"<<reponame<<"' for updates..."<<std::endl;

    Storage::MetadataDB metadataDB(".aigit/metadata.db");
    Storage::ObjectStore objectStore(".aigit");

    std::string jsonResponse=getManifestPull(server, reponame);
    if (jsonResponse.empty()) {
        curl_global_cleanup();
        return false;
    }

    std::map<std::string, std::string> remoteManifest=parseManifestJson(jsonResponse);
    if(remoteManifest.empty()){
        std::cerr<<"[Pull] Remote repository is empty or not found.\n";
        curl_global_cleanup();
        return false;
    }

    std::map<std::string, std::string> missingChunks=filterChunks(remoteManifest, metadataDB);

    if (missingChunks.empty()) {
        std::cout<<"[Pull] Local repository is already up to date."<<std::endl;
        curl_global_cleanup();
        return true;
    }

    std::cout<<"[Pull] Found "<<missingChunks.size()<<" new chunk(s) to pull..."<<std::endl;

    size_t successCount=0;
    for(const auto& [hash, url]:missingChunks){
        fs::path objectPath=fs::path(".aigit")/"objects"/hash.substr(0, 2)/hash.substr(2);

        if(downloadChunkPull(url, objectPath)){
            long long fileSize=static_cast<long long>(fs::file_size(objectPath));
            metadataDB.addObject(hash, fileSize, "chunk");

            std::cout<<"Pulled chunk: "<<hash.substr(0, 8)<<"..."<<std::endl;
            successCount++;
        } 
        else{
            std::cerr<<"Failed to pull chunk: "<<hash.substr(0, 8)<<"..."<<std::endl;
        }
    }

    std::cout<<"[Pull] Complete! ("<<successCount<<"/"<<missingChunks.size()<<" updated)"<<std::endl;

    curl_global_cleanup();
    return successCount==missingChunks.size();
}
}