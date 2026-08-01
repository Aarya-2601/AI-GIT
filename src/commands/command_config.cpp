#include "command_config.hpp"
#include "../core/config.hpp"
#include "../core/filesystem.hpp"

#include <iostream>

namespace fs = std::filesystem;

namespace Commands{
    int runConfig(const std::vector<std::string>& args){
        if(!fs::exists(".aigit")){
            std::cerr<<"error: not an ai-git repository (or any of the parent directories): .aigit"<<std::endl;
            return 1;
        }

        std::string configPath=".aigit/config";
        Core::Config config;
        config.load(configPath);

        //no args so list all stored key/value pairs
        if(args.empty() || args[0]=="--list" || args[0]=="-l"){
            std::cout<<"user.name = "<<config.getAuthorName()<<std::endl;
            std::cout<<"user.email = "<<config.getAuthorEmail()<<std::endl;
            return 0;
        }

        //if user.name mentioned
        if(args.size()==1){
            std::string key=args[0];
            std::string value=config.get(key, "");

            if(value.empty()){
                std::cerr<<"error: key '"<<key<<"' not set."<<std::endl;
                return 1;
            }
            std::cout<<value<<std::endl;
            return 0;
        }

        //set value
        if(args.size()>=2){
            std::string key=args[0];
            std::string value=args[1];
            config.set(key, value);
            config.save(configPath);
            std::cout<<"Set "<<key<<" = "<<value<<std::endl;
            return 0;
        }
        return 0;
    }
}