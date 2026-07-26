#ifndef TREE_HPP
#define TREE_HPP

#include "models/object.hpp"
#include <vector>
#include <string>
#include <map>
#include <memory>

namespace Models{
    struct TreeNode{
        std::string name;
        bool isDirectory;
        std::string hash;std::map<std::string, std::unique_ptr<TreeNode>> children;
        TreeNode(const std::string& n, bool dir): name(n), isDirectory(dir) {}
    };

    struct TreeDef{
        std::string mode; //permissions string for files, dirs, etc
        std::string name; 
        std::string hash; 
        bool isSubtree;   //identifies if the tree points to another tree
    };

    class Tree: public GitObject{
    private:
        std::vector<TreeDef> entries;

    public:
        Tree()=default;

        std::string getTypeString() const override{ 
            return "tree"; 
        }
        //all trees converted into standard ones
        std::string serialize() const override;

        void addEntry(const TreeDef& entry);
        
        const std::vector<TreeDef>& getEntries() const{ 
            return entries; 
        }
    };
}

#endif