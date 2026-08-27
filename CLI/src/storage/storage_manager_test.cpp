#include "storage_manager.hpp"

#include "../core/hashing.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

int main()
{
    try
    {
        std::filesystem::path repoRoot = ".aigit";

        Storage::StorageManager manager(repoRoot);
        manager.initialize();

        std::filesystem::path inputFile =
            "chunk-test.bin";

        std::filesystem::path outputFile =
            "chunk-test-reconstructed.bin";

        if (!std::filesystem::exists(inputFile))
        {
            std::cerr
                << "Test file not found: "
                << inputFile
                << "\n";

            return 1;
        }

        std::cout
            << "Input file: "
            << inputFile
            << "\n";

        std::cout
            << "Size: "
            << std::filesystem::file_size(inputFile)
            << " bytes\n\n";

        std::string manifestId =
            manager.storeFile(inputFile);

        std::cout
            << "Manifest/Object ID: "
            << manifestId
            << "\n";

        manager.retrieveFile(
            manifestId,
            outputFile
        );

        if (!std::filesystem::exists(outputFile))
        {
            throw std::runtime_error(
                "Reconstructed file was not created."
            );
        }

        std::ifstream original(
            inputFile,
            std::ios::binary
        );

        std::ifstream reconstructed(
            outputFile,
            std::ios::binary
        );

        std::string originalData(
            (std::istreambuf_iterator<char>(original)),
            std::istreambuf_iterator<char>()
        );

        std::string reconstructedData(
            (std::istreambuf_iterator<char>(reconstructed)),
            std::istreambuf_iterator<char>()
        );

        std::string originalHash =
            Core::calcSHA256(originalData);

        std::string reconstructedHash =
            Core::calcSHA256(reconstructedData);

        std::cout
            << "\nOriginal SHA-256:      "
            << originalHash
            << "\n";

        std::cout
            << "Reconstructed SHA-256: "
            << reconstructedHash
            << "\n\n";

        if (originalHash != reconstructedHash)
        {
            std::cerr
                << "FAILED: reconstructed file does not match.\n";

            return 1;
        }

        std::cout
            << "SUCCESS: reconstructed file matches original.\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "Test failed: "
            << e.what()
            << "\n";

        return 1;
    }
}