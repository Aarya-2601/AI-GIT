//GET request for all the hashes in backend
//fetch manifest file from PG generated presigned URLs
//return json hashes with URL from backend [hash:url] map
//download binary chunks from MinIO
//MinIO writes to the objects and metadata directory

#ifndef CLONE_HPP
#define CLONE_HPP

#include <string>

namespace Commands{
    bool runClone(const std::string& reponame, const std::string server="http://localhost:3000");
}

#endif