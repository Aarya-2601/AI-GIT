#include "status.hpp"
#include "../core/object_io.hpp"
#include "../helpers/gitutils.hpp"
#include "../storage/storage_manager.hpp"

namespace fs=std::filesystem;

namespace Commands
{
    static void collectHeadEntries(const std::string& treeHash, const std::string& currentPrefix, std::unordered_map<std::string, std::string>& headEntries){
        if(treeHash.empty()) return;

        Models::Tree tree;
        try {
            tree = Core::loadTree(treeHash);
        } catch (const std::exception&) {
            return;
        }

        for(const auto& entry : tree.getEntries()){
            std::string fullPath=currentPrefix.empty() ? entry.name : currentPrefix+"/"+entry.name;

            if(entry.isSubtree){
                collectHeadEntries(entry.hash, fullPath, headEntries);
            }
            else{
                headEntries[fullPath]=entry.hash;
            }
        }
    }

    static std::unordered_map<std::string, std::string> getHeadCommitEntries()
    {
        std::unordered_map<std::string, std::string> headEntries;

        std::string commitHash = Utils::getCurrentCommitHash();
        if(commitHash.empty()) return headEntries;

        try {
            Models::Commit commit = Core::loadCommit(commitHash);
            collectHeadEntries(commit.getTreeHash(), "", headEntries);
        } catch (const std::exception&) {
            return headEntries;
        }

        return headEntries;
    }

    int runStatus()
    {
        if (!fs::exists(".aigit")) // repository check
        {
            std::cerr << "Error: Not an AI-Git repository.\n";
            return 1;
        }

        Core::Index index;
        index.load(".aigit/index"); //index file kholo

        auto headEntries = getHeadCommitEntries();
        
        std::vector<std::pair<std::string, std::string>> stagedFiles; // <type, path>
        std::vector<std::string> modifiedFiles;
        std::vector<std::string> deletedFiles;
        std::vector<std::string> untrackedFiles;
        std::set<std::string> seenDiskFiles;

        for (const auto& [path, entry] : index.getEntries())
        {
            auto headIt = headEntries.find(path);
            if (headIt == headEntries.end()) {
                stagedFiles.push_back({"new file:   ", path});
            } else if (headIt->second != entry.hash) {
                stagedFiles.push_back({"modified:   ", path});
            }
        }

        for (const auto& entry : fs::recursive_directory_iterator("."))
        {      
            if (!entry.is_regular_file()) continue;

            std::string pStr = Utils::normalizePath(entry.path());

            // Skip repository metadata & build output folders
            if (Utils::isIgnoredPath(pStr))
            {
                continue;
            }

            std::string filePath = pStr;
            seenDiskFiles.insert(filePath);

            const auto& indexMap = index.getEntries();
            auto idxIt = indexMap.find(filePath);

            if(idxIt == indexMap.end()) 
            {
                untrackedFiles.push_back(filePath);
            }
            else
            {
                try {
                    // Must match the object ID `add` would compute for
                    // this file's current content, not a different
                    // hashing scheme, or every tracked file would show
                    // as modified.
                    Storage::StorageManager storageManager(".aigit");
                    std::string objectId = storageManager.computeObjectId(entry.path());

                    if (objectId != idxIt->second.hash)
                    {
                        modifiedFiles.push_back(filePath);
                    }
                }
                catch (const std::exception&) {
                    std::cerr << "Error: Failed to open file for reading: " << filePath << std::endl;
                }
            }
        }

        for (const auto& [path, entry] : index.getEntries())
        {
            if (seenDiskFiles.find(path) == seenDiskFiles.end())
            {
                deletedFiles.push_back(path);
            }
        }
           
        //formatting
        std::cout<<"On branch main\n\n";  //abhi ke liye main rakha hai, when we get ai-git branch sorted then we will add branch name here
        bool hasChanges=false;

        //staged files
        if (!stagedFiles.empty()){
            hasChanges=true;
            std::cout<<"Changes to be committed:\n";
            for(const auto& [label, file] : stagedFiles){
                std::cout<<"\t"<<label<<file<<"\n";
            }
            std::cout<<"\n";
        }

        //unstaged modifications and deletions
        if (!modifiedFiles.empty() || !deletedFiles.empty())
        {
            hasChanges=true;
            std::cout<<"Changes not staged for commit:\n";
            std::cout<<"  (use \"ai-git add <file>...\" to update what will be committed)\n";
            for(const auto& file : modifiedFiles){
                std::cout<<"\tmodified:   "<<file<<"\n";
            }
            for(const auto& file : deletedFiles){
                std::cout<<"\tdeleted:    "<<file<<"\n";
            }
            std::cout<<"\n";
        }

        //untracked files
        if (!untrackedFiles.empty()){
            hasChanges=true;
            std::cout<<"Untracked files:\n";
            std::cout<<"  (use \"ai-git add <file>...\" to include in what will be committed)\n";
            for(const auto& file : untrackedFiles)
            {
                std::cout<<"\t"<<file<<"\n";
            }
            std::cout<<"\n";
        }
        
        if (!hasChanges){
            std::cout<<"Nothing to commit, working tree clean\n";
        }
        else if(stagedFiles.empty() && !untrackedFiles.empty()){
            std::cout<<"Nothing added to commit but untracked files present (use \"ai-git add\" to track)\n";
        }
        else if(stagedFiles.empty() && (!modifiedFiles.empty() || !deletedFiles.empty())){
            std::cout<<"No changes added to commit (use \"ai-git add\" and/or \"ai-git commit\")\n";
        }

        return 0;
    }
}