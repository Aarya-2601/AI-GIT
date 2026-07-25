struct TreeNode
{
    std::string name;
    bool isDirectory;

    std::string hash;

    std::map<std::string, std::unique_ptr<TreeNode>> children;

    TreeNode(const std::string& nodeName, bool directory)
        : name(nodeName), isDirectory(directory) {}
};