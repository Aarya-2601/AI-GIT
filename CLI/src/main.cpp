#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include "commands/commands.hpp"

//argument count and argument vector having the number of arguments written in command line
int main(int argc, char* argv[]) {
    //make a vector of strings, each string representing an argument
    std::vector<std::string> args(argv, argv + argc);


    if(args.size()<2){
        std::cerr<<"Usage: ai-git <command> [<args>]"<<std::endl;
        std::cout<< std::endl;
        std::cout<<"Available Commands: "<<std::endl;
        std::cout<<" init: Initialize a new repository"<<std::endl;
        std::cout<<" add: Stage files or model checkpoints"<<std::endl;
        std::cout<<" commit: Commit staged changes"<<std::endl;
        std::cout<<" diff: Semantic AST & SIMD delta model diff"<<std::endl;
        std::cout<<" inspect: Format-aware header inspection"<<std::endl;
        std::cout<<" status: Show working tree status"<<std::endl;
        std::cout<<" log: Display commit log"<<std::endl;
        std::cout<<" push: Push CAS chunks to remote"<<std::endl;
        std::cout<<" pull: Pull changes from remote"<<std::endl;
        std::cout<<" clone: Clone repository"<<std::endl;
        return 1;
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    std::string command=args[1];
    int exitCode = 0;

    if(command=="init"){
        exitCode = Commands::runInit();
    } 

    else if(command=="diff"){
        std::vector<std::string> targets(args.begin()+2, args.end());
        exitCode = Commands::runDiff(targets);
    }

    else if(command=="inspect"){
        if(args.size()<3){
            std::cerr<<"Error: 'inspect' requires a target file."<<std::endl;
            std::cerr<<"Usage: ai-git inspect <file_path>"<<std::endl;
            return 1;
        }
        exitCode = Commands::runInspect(args[2]);
    } 

    else if(command=="add"){
        std::vector<std::string> targets(args.begin()+2, args.end());
        exitCode = Commands::runAdd(targets);
    }

    else if(command=="status"){
        exitCode = Commands::runStatus();
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

        exitCode = Commands::runCommit(commitMessage);
    }

    else if(command=="hash-object"){
        if(args.size()<3){
            std::cerr<<"Error: 'hash-object' requires a valid filename parameter."<<std::endl;
            return 1;
        }
        exitCode = Commands::runHashObject(args[2]);
    }
    
    else if(command=="log"){
        exitCode = Commands::runLog();
    }

    else if(command=="config"){
        std::vector<std::string> configArgs(args.begin()+2, args.end());
        exitCode = Commands::runConfig(configArgs);
    }

    else if(command=="push"){
        //custom server parameter: ai-git push http://localhost:3000
        std::string server=(args.size()>=3)? args[2]:"http://localhost:3000";
        exitCode = Commands::runPush(server);
    }

    else if (command=="pull") {
        //use like ai-git pull <reponame>
        std::string repoName=(args.size()>=3)? args[2]:"default-repo";
        exitCode = Commands::runPull(repoName);
    }
    
    else if(command == "clone") {
        // Usage: ai-git clone <reponame>
        if(args.size()<3){
            std::cerr<<"Error: 'clone' requires a repository name."<<std::endl;
            std::cerr<<"Usage: ai-git clone <repo_name>"<<std::endl;
            return 1;
        }
        std::string repoName=args[2];
        exitCode = Commands::runClone(repoName);
    }

    else{
        std::cerr<<"Error: Command '"<<command<<"' not recognized."<<std::endl;
        return 1;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();

    std::cout << "\n⚡ Done: 'ai-git " << command << "' completed in " 
              << std::fixed << std::setprecision(2) << elapsedSeconds 
              << "s\n" << std::flush;

    return exitCode;
}