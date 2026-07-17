#ifndef COMMIT_HPP
#define COMMIT_HPP

#include "models/object.hpp"
#include <vector>

namespace Models{

    struct CommitMsg{
        std::string name;  //identity
        std::string email;     
        long long timestamp;   
        std::string timezone;  
    };

    class Commit: public GitObject{
    private:
        std::string treeHash;  //pointer to the master snapshot folder 
        std::vector<std::string> parents;  //arrays tracking parent hash
        CommitMsg mauthor;
        CommitMsg mcommitter;
        std::string messagewrite;  //revision history

    public:
        Commit() = default;

        std::string getTypeString() const override{
            return "commit"; 
        }
        
        //generation of readable block
        std::string Models::Commit::serialize() const override;

        //data
        void setTreeHash(const std::string& hash){ 
            treeHash=hash; 
        }
        void addParentHash(const std::string& hash){ 
            parents.push_back(hash); 
        }
        void setAuthor(const CommitMsg& author){ 
            mauthor=author; 
        }
        void setCommitter(const CommitMsg& committer){ 
            mcommitter=committer; 
        }
        void setMessage(const std::string& message){ 
            messagewrite=message; }

        //write parameters
        const std::string& getTreeHash() const{ 
            return treeHash; 
        }
        const std::vector<std::string>& getParentHashes() const{ 
            return parents;
        }
        const CommitMsg& getAuthor() const{ 
            return mauthor; 
        }
        const CommitMsg& getCommitter() const{ 
            return mcommitter; 
        }
        const std::string& getMessage() const{ 
            return messagewrite; 
        }
    };
}

#endif