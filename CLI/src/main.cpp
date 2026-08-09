#include <iostream>
#include <vector>
#include <string>
#include "commands/commands.hpp"

//argument count and agrument vector ahving the number of aruments written in command line
int main(int argc, char* argv[]) {
    //make a vector of strings, each string representing an argument
    std::vector<std::string> args(argv, argv + argc);


    if(args.size()<2){
        std::cerr<<"Usage: ai-git <command> [<args>]"<<std::endl;
        std::cout<< std::endl;
        std::cout<<"Available Commands: "<<std::endl;
        std::cout<<" init: Initialize a a new repository"<<std::endl;
        std::cout<<" hash-object: Compute hash ID and optionally create a blob"<<std::endl;
        std::cout<<" status: Show working tree status"<<std::endl;
        return 1;
    }

    std::string command=args[1];
    if(command=="init"){
        return Commands::runInit();
    } 

    else if(command=="add"){
        std::vector<std::string> targets(args.begin()+2, args.end());
        return Commands::runAdd(targets);
    }

    else if(command=="status"){
        return Commands::runStatus();
    }

    else if(command=="commit"){
        if(args.size()<3){
        std::cerr<<"Error: Commit message required."<<std::endl;
        std::cerr<<"Usage: ai-git commit \"<message>\" or ai-git commit -m \"<message>\""<<std::endl;
        return 1;
        }

        std::string commitMessage;

        //supports ai-git commit -m "msg" and ai-git commit "msg" both
        if(args[2]=="-m"){
            if(args.size()<4){
                std::cerr<<"Error: Missing commit message after -m flag."<<std::endl;
                return 1;
            }
            commitMessage=args[3];
        } 
        else{
            commitMessage=args[2];
        }

        return Commands::runCommit(commitMessage);
    }

    else if(command=="hash-object"){
        if(args.size()<3){
            std::cerr<<"Error: 'hash-object' requires a valid filename parameter."<<std::endl;
            return 1;
        }
        return Commands::runHashObject(args[2]);
    }
    
    else if(command=="log"){
    return Commands::runLog();
    }

    else if(command=="config"){
        std::vector<std::string> configArgs(args.begin()+2, args.end());
        return Commands::runConfig(configArgs);
    }

    else{
        std::cerr<<"Error: Command '"<<command<<"' not recognized."<<std::endl;
        return 1;
    }
}