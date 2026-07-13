#ifndef COMMANDS_H  //if not defined then commands_h will be 
#define COMMANDS_H  //defined as follows

#include <vector>  //include some reqd libraries
#include <string>

namespace Commands{  //organizational namepace box
    int runInit();  //init command to be scheduled for run first
    int runHashObject(const std::string& filePath);  //hash object function
}
#endif