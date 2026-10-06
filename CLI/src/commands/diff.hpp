#ifndef DIFF_HPP
#define DIFF_HPP

#include <vector>
#include <string>

namespace Commands {
    int runDiff(const std::vector<std::string>& targets);
    int runInspect(const std::string& filePath);
}

#endif // DIFF_HPP

