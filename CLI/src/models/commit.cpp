#include "commit.hpp"
#include <sstream>  //sstream is used for ram reading and writing
#include <utility>  //low level handling library 

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
        //root snapshot pointer, treehash likha
        body<< "tree "<< treeHash<< std::endl;
        //track parent commits, empty if initial and multiple if merged, parents likhe
        for(const auto& parent: parents){
            body<< "parent "<< parent<< std::endl;
        }
        //timeline insertion, +0000 tracks standard UTC clock states., author and committer ke details likhe
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

    std::string body = data.substr(headerEnd + 1);  //extract commit body text

    std::stringstream ss(body);

    std::string line;

    while(std::getline(ss, line))  
    {
        if(line.empty())  //encounter empty line toh break
            break;

        if(line.rfind("tree ", 0) == 0)
        {
            commit.setTreeHash(line.substr(5));  //extracts 64 char tree hash present at index 5
        }
        else if(line.rfind("parent ", 0) == 0)
        {
            commit.addParentHash(line.substr(7));  //extracts and records parent commit hashes
        }
        else if(line.rfind("author ", 0) == 0) //reverse find starting from the string, evaluates to true
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

    CommitMsg parseCommitMsg(const std::string& line){  //extracts and records data
        CommitMsg msg;
        size_t emailStart=line.find('<');
        size_t emailEnd=line.find('>');

        if(emailStart != std::string::npos && emailEnd!=std::string::npos && emailEnd>emailStart){  //npos=no position=not found
            msg.name=line.substr(0, emailStart);
            while (!msg.name.empty() && msg.name.back()==' ') {  //trim trailing space before <
                msg.name.pop_back();
            }

            msg.email=line.substr(emailStart+1, emailEnd-emailStart-1);

            std::string rest=line.substr(emailEnd + 1);
            std::stringstream ss(rest);
            ss>>msg.timestamp>>msg.timezone;
        }
        else{
            msg.name=line;  //treats the entire line as the email
            msg.timestamp=0;  //sets to 1 jan 1970
        }

        return msg;
    }
}