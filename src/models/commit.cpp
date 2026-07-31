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
        body<< "committer "<< committer.name<< " "<< committer.email<<"> "<<committer.timestamp<< " "<<committer.timezone<< std::endl;
        body<< std::endl<< message<< std::endl;
        std::string commitContent= body.str();
        //encapsulation of data with a header
        std::string header= "commit "+std::to_string(commitContent.size());
        header.push_back('\0');
        return header + commitContent;
    }

    Commit Commit::deserialize(const std::string& data)
{
    Commit commit;

    size_t headerEnd = data.find('\0');

    if(headerEnd == std::string::npos)
    {
        throw std::runtime_error("Invalid commit object.");
    }

    std::string body = data.substr(headerEnd + 1);

    std::stringstream ss(body);

    std::string line;

    while(std::getline(ss, line))
    {
        if(line.empty())
            break;

        if(line.rfind("tree ", 0) == 0)
        {
            commit.setTreeHash(line.substr(5));
        }
        else if(line.rfind("parent ", 0) == 0)
        {
            commit.addParentHash(line.substr(7));
        }
        else if(line.rfind("author ", 0) == 0)
        {
            commit.setAuthor(parseCommitMsg(line.substr(7)));
        }
        else if(line.rfind("committer ", 0) == 0)
        {
            commit.setCommitter(parseCommitMsg(line.substr(10)));
        }
    }

    std::string message;
    std::string temp;

    while(std::getline(ss, temp))
    {
        if(!message.empty())
            message += "\n";

        message += temp;
    }

    commit.setMessage(message);

    return commit;
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