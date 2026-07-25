#include "core/index.hpp"
#include<iostream>
#include<fstream>
#include<sstream>

namespace Core{

    void Index::load(const std::string& Indexpath){
        entries.clear();
        std::ifstream file(Indexpath);
        if(!file.is_open()){
            return;
        }
        std::string line;
        while(std::getline(file,line)){
            if(line.empty()){
                continue;
            }
            std::istringstream stream(line);
            IndexEntry entry;
            if(stream>>entry.mode>>entry.hash>>entry.path){
                entries[entry.path]=entry;
            }
        }
        std::cout<<"Index loaded from "<<Indexpath<<std::endl;
    }

    void Index::save(const std::string& Indexpath) const{
        std::ofstream file(Indexpath, std::ios::trunc);
        if(!file.is_open()){
            std::cerr<<"Failed to open file for writing: "<<Indexpath<<std::endl;
            return;
        }
        for(const auto& [path, entry]:entries){
            file<<entry.mode<<" "<<entry.hash<<" "<<entry.path<<std::endl;
        }
        std::cout<<"Index saved to "<<Indexpath<<std::endl;
    }

    void Index::addEntry(const IndexEntry& entry){
        entries[entry.path]=entry;
        std::cout<<"Entry added: "<<entry.path <<std::endl;
    }

    void Index::removeEntry(const std::string& path){
        auto it=entries.find(path);
        if(!entries.empty() && it!=entries.end()){
            entries.erase(it);
            std::cout<<"Entry removed: "<<path<<std::endl;
        }
        else{
            std::cout<<"Entry not found at: "<<path<<std::endl;
        }
    }  

    const std::map<std::string, IndexEntry>& Index::getEntries() const{
        for(const auto& entry: entries){
            std::cout<<"Mode: "<<entry.second.mode<<", Hash: "<<entry.second.hash<<", Path: "<<entry.second.path<<std::endl;
        }
        return entries;
    }
}