#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <memory>

#include "commands.hpp"
#include "../core/index.hpp"
#include "../core/config.hpp"
#include "../core/storage.hpp"
#include "../core/compression.hpp"
#include "../core/hashing.hpp"
#include "../models/object.hpp"

#include "../models/tree.hpp"
#include "../models/commit.hpp"

#include "../core/filesystem.hpp"
#include "../helpers/gitutils.hpp"
#include "../models/object.hpp"

namespace Commands
{
    int runCommit(const std::string& message);
}