#ifndef ADD_HPP
#define ADD_HPP

#include <string>
#include <vector>

namespace Commands{
    int runAdd(const std::vector<std::string>& targets);
    int runAdd(const std::string& filePath);
}

#endif