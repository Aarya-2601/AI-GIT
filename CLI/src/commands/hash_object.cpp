#include "hash_object.hpp"
#include "../helpers/ui_theme.hpp"

namespace Commands
{
    int runHashObject(const std::string& filePath, bool verbose)
    {
        UI::initTerminal();

        // Verify whether file exists on disk
        if (!fs::exists(filePath))
        {
            std::cerr << UI::Color::RED << "Error: File does not exist: " << filePath << UI::Color::RESET << std::endl;
            return 1;
        }

        if (!fs::is_regular_file(filePath))
        {
            std::cerr << UI::Color::RED << "Error: Path specified is not a regular file: " << filePath << UI::Color::RESET << std::endl;
            return 1;
        }

        try
        {
            // Same raw-bytes / chunk-manifest hashing and storage path as
            // `add` (StorageManager::storeFile), so hash-object prints the
            // same ID `add` would record for identical content.
            Storage::StorageManager storageManager(".aigit");
            std::string objectId = storageManager.storeFile(filePath);

            if (!verbose)
            {
                std::cout << objectId << std::endl;
                return 0;
            }

            uintmax_t fileSize = fs::file_size(filePath);
            std::string storageMode = (fileSize > 256 * 1024) ? "FastCDC Chunked Manifest" : "Direct Blob";

            std::cout << "\n";
            std::cout << UI::Color::CYAN << UI::Color::BOLD << "  🦎 FASTCDC CAS OBJECT INSPECTOR" << UI::Color::RESET << "\n";
            std::cout << UI::Color::BORDER << "  ───────────────────────────────────────────────────────────────────\n";
            std::cout << "  │ " << UI::Color::SLATE << "File Path   : " << UI::Color::WHITE << filePath << UI::Color::RESET << "\n";
            std::cout << "  │ " << UI::Color::SLATE << "File Size   : " << UI::Color::TEAL << UI::formatBytes(fileSize) << " (" << fileSize << " bytes)" << UI::Color::RESET << "\n";
            std::cout << "  │ " << UI::Color::SLATE << "Storage Mode: " << UI::Color::GREEN << storageMode << UI::Color::RESET << "\n";
            std::cout << "  │ " << UI::Color::SLATE << "Blake3 Hash : " << UI::Color::CYAN << UI::Color::BOLD << objectId << UI::Color::RESET << "\n";
            if (objectId.size() >= 4)
            {
                std::cout << "  │ " << UI::Color::SLATE << "Object Store: " << UI::Color::DARK_SLATE << ".aigit/objects/" << objectId.substr(0, 2) << "/" << objectId.substr(2) << UI::Color::RESET << "\n";
            }
            std::cout << UI::Color::BORDER << "  ───────────────────────────────────────────────────────────────────\n";
            std::cout << "  " << UI::Color::GREEN << "✔ Computed object ID: " << UI::Color::CYAN << objectId << UI::Color::RESET << "\n\n";

            return 0;
        }
        catch(const std::exception& e)
        {
            std::cerr << UI::Color::RED << "Execution Exception: " << e.what() << UI::Color::RESET << std::endl;
            return 1;
        }
    }
}