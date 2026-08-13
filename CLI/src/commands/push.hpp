//read all local hashes
//send a POST request with the chunks we have
//request reaches postgreSQL and it check the missing or modified chunks
//returns a presigned MinIo url for the missing chunks
//read the raw bytes of missing chunks 
//send an HTTP PUT request to minIO using libcurl

#ifndef PUSH_HPP
#define PUSH_HPP

#include <string>
namespace Commands{
    bool runPush(const std::string server="http://localhost/3000");
}

#endif