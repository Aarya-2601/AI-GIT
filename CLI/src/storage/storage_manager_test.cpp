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
        std::filesystem::path repoRoot = ".aigit/cas";

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

        manager.restoreFile(
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

        // Dedup: storing a second file that shares most of its content
        // with the first (small edit, same chunk boundaries elsewhere)
        // must skip re-writing the shared chunks -- on-disk growth
        // should be far smaller than the second file's own size.

        std::string dedupInputData = originalData;

        if (dedupInputData.size() > 10)
        {
            dedupInputData[dedupInputData.size() / 2] ^= 0xFF;
            dedupInputData[dedupInputData.size() / 2 + 1] ^= 0xFF;
        }

        std::filesystem::path dedupInputFile = "chunk-test-dedup.bin";

        {
            std::ofstream out(dedupInputFile, std::ios::binary);
            out.write(dedupInputData.data(), static_cast<std::streamsize>(dedupInputData.size()));
        }

        std::uintmax_t sizeBeforeDedup =
            std::filesystem::exists(repoRoot / "objects")
                ? [&]{
                      std::uintmax_t total = 0;
                      for (const auto& entry : std::filesystem::recursive_directory_iterator(repoRoot / "objects"))
                      {
                          if (entry.is_regular_file()) total += entry.file_size();
                      }
                      return total;
                  }()
                : 0;

        manager.storeFile(dedupInputFile);

        std::uintmax_t sizeAfterDedup = [&]{
            std::uintmax_t total = 0;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(repoRoot / "objects"))
            {
                if (entry.is_regular_file()) total += entry.file_size();
            }
            return total;
        }();

        std::uintmax_t growth = sizeAfterDedup - sizeBeforeDedup;
        std::uintmax_t dedupInputSize = dedupInputData.size();

        std::cout
            << "Dedup: second (near-identical) file is " << dedupInputSize
            << " bytes; store grew by " << growth << " bytes.\n";

        if (growth >= dedupInputSize)
        {
            std::cerr
                << "FAILED: storing a near-identical file did not "
                << "dedup shared chunks (store grew by the full file "
                << "size or more).\n";

            return 1;
        }

        std::cout
            << "SUCCESS: near-identical file's shared chunks were "
            << "deduped (store growth well under the file's own size).\n";

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