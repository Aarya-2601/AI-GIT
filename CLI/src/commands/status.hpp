#ifndef STATUS_HPP
#define STATUS_HPP

#include "commands.hpp"
#include "../core/index.hpp"
#include "../core/hashing.hpp"
#include "../core/storage.hpp"
#include "../core/compression.hpp"
#include "../core/filesystem.hpp"
#include "../models/blob.hpp"

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <set>
#include <algorithm>
#include <iomanip>

namespace Commands{
    int runStatus();
}

#endif