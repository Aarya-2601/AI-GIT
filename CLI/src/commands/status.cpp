#include "status.hpp"

namespace fs=std::filesystem;

namespace Commands
{   
    static std::string normalizePath(const fs::path& p){
        std::string pathStr=p.generic_string();
        std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

        if(pathStr.rfind("./", 0) == 0)
        {
            pathStr=pathStr.substr(2);
        }
        //carriage return will not turn into a newline character
        while(!pathStr.empty() && (pathStr.back() == '\r' || pathStr.back() == '\n' || pathStr.back() == ' '))
        {
            pathStr.pop_back();
        }
        return pathStr;
    }

    static void collectHeadEntries(const std::string& treeHash, const std::string& currentPrefix, std::unordered_map<std::string, std::string>& headEntries){
        if(treeHash.empty()) return;

        std::string compressedTree=Core::Storage::readObject(treeHash);
        if(compressedTree.empty()) return;

        std::string rawTree=Core::decompressData(compressedTree);
        if(rawTree.empty()) return;

        size_t nullPos=rawTree.find('\0');
        if(nullPos == std::string::npos) return;

        std::string body=rawTree.substr(nullPos + 1);
        size_t i=0;
        while(i < body.size()){
            size_t spacePos=body.find(' ', i);
            if(spacePos == std::string::npos) break;
            std::string mode=body.substr(i, spacePos-i);

            size_t nullEntryPos=body.find('\0', spacePos+1);
            if (nullEntryPos == std::string::npos) break;
            std::string name=body.substr(spacePos+1, nullEntryPos-(spacePos + 1));

            if(nullEntryPos+1+32 > body.size()) break;
            std::string binaryHash=body.substr(nullEntryPos+1, 32);
            i=nullEntryPos+1+32;

            std::stringstream ss;
            for(unsigned char c : binaryHash){
                ss<<std::hex<<std::setw(2)<<std::setfill('0')<<static_cast<int>(c);
            }
            std::string hexHash=ss.str();

            std::string fullPath=currentPrefix.empty() ? name : currentPrefix+"/"+name;

            if(mode == "040000"){
                collectHeadEntries(hexHash, fullPath, headEntries);
            } 
            else{
                headEntries[fullPath]=hexHash;
            }
        }
    }

    static std::unordered_map<std::string, std::string> getHeadCommitEntries()
    {
        std::unordered_map<std::string, std::string> headEntries;
        if(!fs::exists(".aigit/HEAD")) return headEntries;

        std::ifstream headFile(".aigit/HEAD");
        if(!headFile.is_open()) return headEntries;

        std::string refLine;
        std::getline(headFile, refLine);
        headFile.close();

        while(!refLine.empty() && (refLine.back()=='\r' || refLine.back()=='\n' || refLine.back()==' ')){
            refLine.pop_back();
        }

        if(refLine.rfind("ref: ", 0) != 0) return headEntries;

        std::string refPath = ".aigit/"+refLine.substr(5);
        if(!fs::exists(refPath)) return headEntries;

        std::ifstream branchFile(refPath);
        if(!branchFile.is_open()) return headEntries;

        std::string commitHash;
        std::getline(branchFile, commitHash);
        branchFile.close();

        while(!commitHash.empty() && (commitHash.back() == '\r' || commitHash.back() == '\n' || commitHash.back() == ' ')){
            commitHash.pop_back();
        }

        if(commitHash.empty()) return headEntries;

        std::string compressedCommit=Core::Storage::readObject(commitHash);
        if(compressedCommit.empty()) return headEntries;

        std::string rawCommit=Core::decompressData(compressedCommit);
        if(rawCommit.empty()) return headEntries;

        size_t treePos = rawCommit.find("tree ");
        if(treePos != std::string::npos){
            size_t start = treePos + 5;
            size_t end = rawCommit.find_first_of("\r\n", start);
            if(end != std::string::npos){
                std::string rootTreeHash = rawCommit.substr(start, end - start);
                collectHeadEntries(rootTreeHash, "", headEntries);
            }
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

            std::string pStr = normalizePath(entry.path().generic_string());
            
            // Skip repository metadata & build output folders
            if (pStr.find(".aigit") != std::string::npos || pStr.find("build/") != std::string::npos || pStr.find(".git") != std::string::npos || pStr.find(".vscode/") != std::string::npos || pStr.find("vcpkg/") != std::string::npos)
            {
                continue;
            }

            std::string filePath = normalizePath(entry.path());
            seenDiskFiles.insert(filePath);

            const auto& indexMap = index.getEntries();
            auto idxIt = indexMap.find(filePath);

            if(idxIt == indexMap.end()) 
            {
                untrackedFiles.push_back(filePath);
            }
            else 
            {
                std::ifstream inFile(entry.path(), std::ios::binary);
                if (inFile.is_open()) {
                    std::stringstream buffer;
                    buffer << inFile.rdbuf();
                    std::string fileContent = buffer.str();
                    inFile.close();

                    Models::Blob blobObject(fileContent);
                    std::string storePayload = blobObject.serialize();
                    std::string sha256Hash = Core::calcSHA256(storePayload);

                    if (sha256Hash.empty()) {
                        std::cerr << "Error: Cryptographic hashing mechanism failed." << std::endl;
                        return 1; 
                    }

                    if (sha256Hash != idxIt->second.hash)
                    {
                        modifiedFiles.push_back(filePath);
                    }
                }
                else {
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