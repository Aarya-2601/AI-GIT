#pragma once

#include <string>
#include <map>
#include <memory>

namespace Models
{
    struct TreeNode
    {
        std::string name;
        bool isDirectory;
        std::string hash;

        std::map<std::string, std::unique_ptr<TreeNode>> children;

        TreeNode(const std::string& n, bool dir) 
            : name(n), isDirectory(dir) // contructor 
        {
        }
    };
}