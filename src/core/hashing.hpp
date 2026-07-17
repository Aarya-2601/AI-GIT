#ifndef HASHING_HPP  //include guard prevents double inclusion errors
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