#ifndef REFS_H
#define REFS_H

#include <string>

namespace Core{
    namespace Refs{
        static std::string getHead(const std::string& repoPath);
        static std::string getcurrentCommitHash(const std::string& repoPath);
        static void updateBranch(const std::string& branch, const std::string& commitHash, const std::string& repoPath);
        static void setHead(const std::string& targetRef, const std::string& repoPath=".aigit");
    };
}

#endif