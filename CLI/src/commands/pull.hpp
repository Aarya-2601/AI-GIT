//send a GET request to get a list of chunks and presigned urls
////deduplication so comapre local with remote and discard those we already have
//download missing chunks from MinIo using libcurl
//update local db

#ifndef PULL_HPP
#define PULL_HPP

#include <string>

namespace Commands {
    bool runPull(const std::string& repoName="default-repo", const std::string& server="http://localhost:3000");
}

#endif