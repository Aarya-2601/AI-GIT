#include "core/refs.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
namespace fs=std::filesystem;

namespace Core{
    namespace Refs{
        static std::string trim(const std::string& str){
            size_t first=str.find_first_not_of(" \t\r\n");
            if(first==std::string::npos) return "";
            size_t last=str.find_last_not_of(" \t\r\n");
            return str.substr(first, (last-first+1));
        }

        std::string getHead(){
            fs::path headpath=fs::path(repoPath)/"HEAD";
            std::ifstream file(headPath);
            if(!file.is_open()){
                return "";
            }
            std::string line;
            std::getline(file, line);
            return trim(line);
        }

        std::string getcurrentCommitHash(){
            std::string headContent=getHEAD(repoPath);
            if(headContent.empty()){
                return "";
            }
            //if head points to a symbolic reference link
            const std::string refPrefix="ref: ";
            if(headContent.rfind(refPrefix, 0)==0){ 
                std::string refPath=headContent.substr(refPrefix.length());
                fs::path fullRefPath=fs::path(repoPath)/refPath;

                std::ifstream refFile(fullRefPath);
                if (!refFile.is_open()) {
                    //branch ref file does not exist on disk yet
                    return ""; 
                }

                std::string commitHash;
                std::getline(refFile, commitHash);
                return trim(commitHash);
            }
            //HEAD directly stores a commit SHA hash
            return headContent;
            }

            void updateBranch(const std::string& branchName, const std::string& commitHash, const std::string& repoPath){
            fs::path branchPath=fs::path(repoPath)/"refs"/"heads"/branchName;
            
            // Ensure .aigit/refs/heads directory tree exists on disk
            fs::create_directories(branchPath.parent_path());

            std::ofstream file(branchPath, std::ios::trunc);
            if (!file.is_open()) {
                std::cerr<<"Error: Could not update branch "<<branchName<<" at "<<branchPath<<std::endl;
                return;
            }
            file<<commitHash<<std::endl;
        }

        void setHead(const std::string& targetRef){
            fs::path headPath=fs::path(repoPath)/"HEAD";
            fs::create_directories(headPath.parent_path());

            std::ofstream file(headPath, std::ios::trunc);
            if(!file.is_open()){
                std::cerr<<"Error: Could not write to HEAD at "<<headPath<<std::endl;
                return;
            }
            // Formats input automatically whether given "main", "refs/heads/main", or "ref: refs/heads/main"
            std::string formattedRef=targetRef;
            if(targetRef.rfind("ref: ", 0)!=0 && targetRef.rfind("refs/", 0)==0){
                formattedRef="ref: "+targetRef;
            } 
            else if(targetRef.rfind("ref: ", 0)!=0 && targetRef.length()<40){
                formattedRef="ref: refs/heads/"+targetRef;
            }
            file<<formattedRef<<std::endl;
        }
    }
}