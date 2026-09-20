#pragma once

#include <filesystem>
#include <string>

namespace Utils
{
    std::string getBranchCommitHash(const std::string& branchName);
    std::string getCurrentBranchName();

    std::string getCurrentCommitHash();



    bool branchExists(const std::string& branchName);

    // Writes `commitHash` into refs/heads/<branchName>, creating the
    // refs/heads directory if needed. Used whenever a branch pointer is
    // created or advanced (branch, commit, ...).
    void writeBranchRef(const std::string& branchName, const std::string& commitHash);

    // Points HEAD at refs/heads/<branchName> (used by checkout).
    void setHeadToBranch(const std::string& branchName);

    // Converts a filesystem path to the forward-slash, repo-relative form
    // used as index/tree keys throughout the codebase (strips a leading
    // "./" and trailing whitespace/CR). Shared by add/status so the same
    // path always normalizes identically in both.
    std::string normalizePath(const std::filesystem::path& p);

    // True if `normalizedPath` falls under a directory ai-git never tracks
    // (.aigit, build/, .git, .vscode/, vcpkg/).
    bool isIgnoredPath(const std::string& normalizedPath);
}