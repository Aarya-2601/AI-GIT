#ifndef COMMIT_HPP
#define COMMIT_HPP

#include "models/object.hpp"
#include <vector>
#include <string>

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
        CommitMsg author;
        CommitMsg committer;
        std::string message;  //revision history

    public:
        Commit()= default;
        Commit(std::string committreeHash, std::vector<std::string> commitparents, CommitMsg commitauthor, CommitMsg commitcommitter, std::string commitmessage, long long timestamp, std::string timezone);

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
        void setAuthor(const CommitMsg& commitauthor){ 
            author=commitauthor; 
        }
        void setCommitter(const CommitMsg& commitcommitter){ 
            committer=commitcommitter; 
        }
        void setMessage(const std::string& commitmessage){ 
            message=commitmessage; }

        //write parameters
        const std::string& getTreeHash() const{ 
            return treeHash; 
        }
        const std::vector<std::string>& getParentHashes() const{ 
            return parents;
        }
        const CommitMsg& getAuthor() const{ 
            return author; 
        }
        const CommitMsg& getCommitter() const{ 
            return committer; 
        }
        const std::string& getMessage() const{ 
            return message; 
        }
    };
}

#endif