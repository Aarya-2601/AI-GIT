#include "filesystem.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Core
{

std::string readFileToString(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);

    if (!input.is_open())
    {
        throw std::runtime_error(
            "Could not open file: " + path.string()
        );
    }

    std::stringstream buffer;
    buffer << input.rdbuf();

    return buffer.str();
}

}
