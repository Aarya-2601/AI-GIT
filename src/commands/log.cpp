#include "commands/log.hpp"

namespace fs=std::filesystem;
using namespace std;

//theres no input in this everythings from head and refs folders 

namespace
{   
    // first thing is to know where the latest pointer is at (which branch) and get the latest commit hash
    std::string getCurrentBranchRef()
{
    std::ifstream headFile(".aigit/HEAD");

    if (!headFile.is_open())
    {
        throw std::runtime_error("Failed to open HEAD.");
    }

    std::string refLine;
    std::getline(headFile, refLine);
    headFile.close();

    while (!refLine.empty() && (refLine.back() == '\r' || refLine.back() == '\n' || refLine.back() == ' '))
    {
        refLine.pop_back();
    }

    if (refLine.substr(0, 5) != "ref: ")
    {
        throw std::runtime_error("Invalid HEAD format.");
    }

    return refLine.substr(5);
}

//latest commit hash from the current branch
std::string getCurrentCommitHash(const std::string& branchRef)
{   
    fs::path refPath = fs::path(".aigit") / branchRef;

    if (!fs::exists(refPath))
    {
        return "";
    }

    std::ifstream branchFile(refPath);

    if (!branchFile.is_open())
    {
        throw std::runtime_error("Failed to open branch.");
    }

    std::string commitHash;
    std::getline(branchFile, commitHash);

    branchFile.close();

    return commitHash;
}

Models::Commit loadCommit(const std::string& hash)
{
    Models::Commit commit;

    // Split hash into folder and filename & open commit object and read entire object
    std::string compressedData = Core::Storage::readObject(hash);
    if (compressedData.empty())
    {
        throw std::runtime_error("Failed to read commit object from storage: " + hash);
    }

    std::string rawObject = Core::decompressData(compressedData);

    if (rawObject.empty())
    {
        throw std::runtime_error("Failed to decompress commit object: " + hash);
    }

    // Find end of header ("commit <size>\0")
    size_t nullPos = rawObject.find('\0');

    if (nullPos == std::string::npos)
    {
        throw std::runtime_error("Invalid commit object payload.");
    }

    //remove header and parse payload
    std::string payload = rawObject.substr(nullPos + 1);
    std::stringstream ss(payload);
    std::string line;

    while (std::getline(ss, line))
    {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        {
            line.pop_back();
        }

        if (line.empty())
        {
            break; //Header section end
        }

        if (line.rfind("tree ", 0) == 0)
        {
            commit.setTreeHash(line.substr(5));
        }
        else if (line.rfind("parent ", 0) == 0)
        {
            commit.addParentHash(line.substr(7));
        }
        else if (line.rfind("author ", 0) == 0)
        {
            commit.setAuthor(Models::parseCommitMsg(line.substr(7)));
        }
        else if (line.rfind("committer ", 0) == 0)
        {
            commit.setCommitter(Models::parseCommitMsg(line.substr(10)));
        }
    }

    std::string message;
    while (std::getline(ss, line))
    {
        while (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        message += line + "\n";
    }

    while (!message.empty() && (message.back() == '\n' || message.back() == '\r'))
    {
        message.pop_back();
    }

    commit.setMessage(message);

    return commit;

}

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
        // Find current branch
        std::string branchRef = getCurrentBranchRef();

        // Get latest commit on that branch
        std::string commitHash = getCurrentCommitHash(branchRef);

        // Traverse commit history
        while (!commitHash.empty())
        {
            Models::Commit commit = loadCommit(commitHash);

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