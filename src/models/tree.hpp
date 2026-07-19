#ifndef TREE_HPP
#define TREE_HPP

#include "models/object.hpp"
#include <vector>

namespace Models{
    struct TreeDef{
        std::string mode; //permissions string for files, dirs, etc
        std::string name; 
        std::string hash; 
        bool isSubtree;   //identifies if the treee points to another tree
    };

    class Tree: public GitObject{
    private:
        std::vector<TreeDef> mentries;

    public:
        Tree()=default;

        std::string getTypeString() const override{ 
            return "tree"; 
        }
        
        //all trees converted into standard ones
        std::string Models::Tree::serialize() const override;

        void Models::Tree::addEntry(const TreeDef& entry);
        const std::vector<TreeDef>& getEntries() const{ 
            return mentries; 
        }
    };
}

#endif