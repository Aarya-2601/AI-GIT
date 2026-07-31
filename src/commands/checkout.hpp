#include "../models/commit.hpp"

#ifndef CHECKOUT_HPP
#define CHECKOUT_HPP

#include <string>

namespace Commands
{
    int runCheckout(const std::string& branchName);
}

#endif