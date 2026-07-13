//include guard prevents double inclusion errors
#ifndef HASHING_HPP
#define HASHING_HPP

//other headers this file needs
#include <string>

//namespace is an organization box
namespace Core{
    //function declarations
    std::string calcSHA256(const std::string &content);
}
//end header files
#endif