#include "commit.hpp"
#include <sstream>
#include <utility>  //low level library 

namespace Models{
    Commit::Commit(std::string committreeHash, std::vector<std::string> commitparents, CommitMsg commitauthor, CommitMsg commitcommitter, std::string commitmessage, long long committimestamp, std::string committimezone)
        : treeHash(std::move(committreeHash)), parents(std::move(commitparents)), author(std::move(commitauthor)), committer(std::move(commitcommitter)), message(std::move(commitmessage)){
            author.timestamp=committimestamp;
            author.timezone=committimezone;
            committer.timestamp=committimestamp;
            committer.timezone=committimezone;
        }

    std::string Commit::serialize() const{
        std::ostringstream body;  //output string stream
        //root snapshot pointer
        body<< "tree "<< treeHash<< std::endl;
        //track parent commits, empty if initial and multiple if merged
        for(const auto& parent: parents){
            body<< "parent "<< parent<< std::endl;
        }
        //timeline insertion, +0000 tracks standard UTC clock states.
        body<< "author "<< author.name<< " "<< author.email<<"> "<< author.timestamp<<" " << author.timezone<< std::endl;
        body<< "committer "<< committer.name<< " "<< committer.email<< "> "<<committer.timestamp<< " "<<committer.timezone<< std::endl;
        body<< std::endl<< message<< std::endl;
        std::string commitContent= body.str();
        //encapsulation of data with a header
        std::string header= "commit "+std::to_string(commitContent.size());
        header.push_back('\0');
        return header + commitContent;
    }

    CommitMsg parseCommitMsg(const std::string& line){
        CommitMsg msg;
        size_t emailStart=line.find('<');
        size_t emailEnd=line.find('>');

        if(emailStart != std::string::npos && emailEnd!=std::string::npos && emailEnd>emailStart){
            msg.name=line.substr(0, emailStart);
            while (!msg.name.empty() && msg.name.back()==' ') {
                msg.name.pop_back();
            }

            msg.email=line.substr(emailStart+1, emailEnd-emailStart-1);

            std::string rest=line.substr(emailEnd + 1);
            std::stringstream ss(rest);
            ss>>msg.timestamp>>msg.timezone;
        }
        else{
            msg.name=line;
            msg.timestamp=0;
        }

        return msg;
    }
}