#pragma once

#include "commands.hpp"
#include "core/storage.hpp"
#include "core/compression.hpp"
#include "core/filesystem.hpp"
#include "models/commit.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>

namespace Commands
{
    int runLog();
}