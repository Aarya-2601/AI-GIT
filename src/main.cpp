#include <iostream>
#include <vector>
#include <string>
#include "commands/commands.hpp"

int main(int argc, char* argv[]) {
    // Pack raw environment data immediately into a manageable string vector
    std::vector<std::string> args(argv, argv + argc);

    if (args.size() < 2) {
        std::cerr << "Usage: " << args[0] << " <init | hash-object> [args]\n";
        return 1;
    }

    std::string command = args[1];

    if (command == "init") {
        return Commands::runInit();
    } 
    else if (command == "hash-object") {
        if (args.size() < 3) {
            std::cerr << "Error: 'hash-object' requires a valid filename parameter.\n";
            return 1;
        }
        return Commands::runHashObject(args[2]);
    } 
    else {
        std::cerr << "Error: Command '" << command << "' not recognized.\n";
        return 1;
    }
}