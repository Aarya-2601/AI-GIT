#include "core/config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Core{
    
    void Config::load(const std::string& configPath){
        settings.clear();
        std::ifstream file(configPath);
        if(!file.is_open()){
            return;
        }

        std::string line;
        std::string currentSection="";

        while(std::getline(file, line)){
            //remove leading and trailing whitespace
            size_t first=line.find_first_not_of(" \t\r\n");
            //blank line
            if(first==std::string::npos) continue;
            size_t last=line.find_last_not_of(" \t\r\n");
            line=line.substr(first, (last-first+1));

            //skipping comment lines
            if(line[0]=='#' || line[0]==';') continue;

            //[user]
            if(line.front()=='[' && line.back()==']'){
                currentSection=line.substr(1, line.size()-2);
            } 
            //key=value
            else{
                size_t eqPos=line.find('=');
                if(eqPos!=std::string::npos){
                    std::string key=line.substr(0, eqPos);
                    std::string value=line.substr(eqPos+1);

                    //remove whitespace
                    key.erase(key.find_last_not_of(" \t")+1);
                    key.erase(0, key.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t")+1);
                    value.erase(0, value.find_first_not_of(" \t"));

                    std::string fullKey=currentSection.empty() ? key:currentSection+"."+key;
                    settings[fullKey]=value;
                }
            }
        }
    }

    void Config::save(const std::string& configPath) const{
        std::ofstream file(configPath, std::ios::trunc);
        if(!file.is_open()){
            std::cerr<<"Failed to open config file for writing: "<<std::endl;
        }

        for(const auto& [key,value] :settings){
            file<<key<<" = "<<value<<std::endl;
        }

    }

    void Config::set(const std::string& key, const std::string& value){
        settings[key]=value;
    }

    std::string Config::get(const std::string& key, const std::string& defaultValue="") const{
        auto it=settings.find(key);
        if(it!=settings.end()){
            return it->second;
        }
        return defaultValue;
    }

    std::string Config::getAuthorName() const{
        return get("user.name", "Unknown User");
    }

    std::string Config::getAuthorEmail() const{
        return get("user.email", "unknown@example.com");
    }
}