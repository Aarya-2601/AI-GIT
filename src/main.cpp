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
    else if(command=="hash-object"){
        if (args.size()<3) {
            std::cerr<<"Error: 'hash-object' requires a valid filename parameter."<<std::endl;
            return 1;
        }
        return Commands::runHashObject(args[2]);
    } 
    else if(command=="status"){
        return Commands::runStatus();
    }
    else{
        std::cerr<<"Error: Command '" << command << "' not recognized.\n";
        return 1;
    }
}