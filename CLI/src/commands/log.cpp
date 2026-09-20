#include "log.hpp"
#include "../core/object_io.hpp"
#include "../helpers/gitutils.hpp"

namespace fs=std::filesystem;
using namespace std;

//theres no input in this everythings from head and refs folders

namespace
{
void printCommit(const Models::Commit& commit, const std::string& commitHash)
{
   std::cout << "Commit: "
              << commitHash
              << std::endl;

    std::cout << "Tree: "
              << commit.getTreeHash()
              << std::endl;

    const auto& parents = commit.getParentHashes();
    for (const auto& parent : parents)
    {
        if (!parent.empty())
        {
            std::cout << "Parent: "
                      << parent
                      << std::endl;
        }
    }

    const auto& author = commit.getAuthor();
    std::cout << "Author: ";
    if (!author.email.empty())
    {
        std::cout << author.name << " <" << author.email << ">";
    }
    else
    {
        std::cout << author.name;
    }
    std::cout << std::endl;

    const auto& committer = commit.getCommitter();
    std::cout << "Committer: ";
    if (!committer.email.empty())
    {
        std::cout << committer.name << " <" << committer.email << ">";
    }
    else
    {
        std::cout << committer.name;
    }
    std::cout << std::endl;

    std::cout << "\nMessage:\n"
              << commit.getMessage()
              << std::endl;
    std::cout << std::endl;
}
}

namespace Commands
{
int runLog()
{
    try
    {   
        if (!fs::exists(".aigit"))
        {
            std::cerr << "fatal: not an ai-git repository (or any of the parent directories): .aigit" << std::endl;
            return 1;
        }
        // Find current branch's latest commit
        std::string commitHash = Utils::getCurrentCommitHash();

        // Traverse commit history
        while (!commitHash.empty())
        {
            Models::Commit commit = Core::loadCommit(commitHash);

            printCommit(commit, commitHash);

            const auto& parents = commit.getParentHashes();
            if (!parents.empty())
            {
                commitHash = parents[0];
            }
            else
            {
                commitHash = "";
            }
        }
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
}