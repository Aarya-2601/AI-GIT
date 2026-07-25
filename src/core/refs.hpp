#ifndef REFS_H
#define REFS_H

#include <string>

namespace Core{
    namespace Refs{
        static std::string getHead();
        static std::string getcurrentCommitHash();
        static void updateBranch(const std::string& branch, const std::string& commitHash);
        static void setHead(const std::string& targetRef, const std::string& repoPath=".aigit");
    };
}

#endif