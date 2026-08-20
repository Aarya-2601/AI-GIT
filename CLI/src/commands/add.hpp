#ifndef ADD_HPP
#define ADD_HPP

#include "commands.hpp"
#include "../core/hashing.hpp"
#include "../core/filesystem.hpp"

#include "../models/blob.hpp"
#include "../core/compression.hpp"
#include "../core/storage.hpp"
#include "../core/index.hpp"
#include "../models/object.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>
#include <vector>

namespace Commands
{
    int runAdd(const std::vector<std::string>& targets);
    int runAdd(const std::string& filePath);
}

#endif