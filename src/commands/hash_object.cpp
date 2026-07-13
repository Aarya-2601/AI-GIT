#include "commands.hpp"
#include "core/hashing.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace Commands {
    int runHashObject(const std::string& filePath) {
        // 1. Verify that the target file actually exists on the disk
        if (!fs::exists(filePath)) {
            std::cerr << "Error: File does not exist: " << filePath << "\n";
            return 1;
        }
        if (!fs::is_regular_file(filePath)) {
            std::cerr << "Error: Path specified is not a regular file: " << filePath << "\n";
            return 1;
        }

        // 2. Read the raw content of the file in binary mode
        std::ifstream inFile(filePath, std::ios::binary);
        if (!inFile.is_open()) {
            std::cerr << "Error: Could not open file for reading: " << filePath << "\n";
            return 1;
        }

        std::stringstream buffer;
        buffer << inFile.rdbuf();
        std::string fileContent = buffer.str();
        inFile.close();

        // 3. Replicate Git's official object format structure
        // Git doesn't just hash raw file text. It prepends a header: "blob <content_size>\0"
        // The '\0' null byte acts as a strict separator between the header metadata and the payload.
        std::string gitHeader = "blob " + std::to_string(fileContent.size()) + '\0';
        std::string storePayload = gitHeader + fileContent;

        // 4. Compute the OpenSSL SHA-256 hash using your Core module
        std::string sha256Hash = Core::calcSHA256(storePayload);
        if (sha256Hash.empty()) {
            std::cerr << "Error: Cryptographic hashing mechanism failed.\n";
            return 1;
        }

        // 5. Determine the split-path database storage architecture
        // The first 2 characters become the subfolder, remaining 62 become the file name.
        fs::path objectsRoot = ".git/objects";
        std::string dirPrefix = sha256Hash.substr(0, 2);
        std::string fileSuffix = sha256Hash.substr(2);

        fs::path objectFolder = objectsRoot / dirPrefix;
        fs::path objectFile = objectFolder / fileSuffix;

        try {
            // 6. Ensure the 2-character subfolder exists inside the database
            fs::create_directories(objectFolder);

            // 7. Write the header + file data package to the object store
            std::ofstream outFile(objectFile, std::ios::binary);
            if (!outFile.is_open()) {
                std::cerr << "Error: Failed to write object path to disk.\n";
                return 1;
            }

            outFile << storePayload;
            outFile.close();

            // 8. Print out the full calculated hash string to standard output
            // This mirrors real Git behavior perfectly!
            std::cout << sha256Hash << "\n";
            return 0;

        } catch (const fs::filesystem_error& e) {
            std::cerr << "Database Write Exception: " << e.what() << "\n";
            return 1;
        }
    }
}