#include "push.hpp"
#include "../sync/cas_sync_engine.hpp"
#include "../core/index.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

namespace Commands {

bool runPush(const std::string& serverUrl) {
    if (!fs::exists(".aigit")) {
        std::cerr << "Error: Not an AI-Git repository." << std::endl;
        return false;
    }

    Core::Index index;
    index.load(".aigit/index");
    if (index.getEntries().empty()) {
        std::cerr << "Warning: No staged files in index to push." << std::endl;
        return true;
    }

    std::cout << "Connecting to Remote CAS at: " << serverUrl << " ...\n";
    std::string remoteTarget = serverUrl;
    if (remoteTarget.find("://") == std::string::npos) {
        remoteTarget = "http://" + remoteTarget;
    }

    auto remote = Core::Sync::createCASRemote(remoteTarget, "aigit-models");
    if (!remote) {
        std::cerr << "Error: Failed to initialize CAS remote storage client." << std::endl;
        return false;
    }

    size_t totalFiles = 0;
    size_t totalUploadedChunks = 0;
    size_t totalSkippedChunks = 0;
    size_t totalBytesUploaded = 0;
    size_t totalBytesSaved = 0;

    Core::Sync::CASSyncEngine syncEngine;
    for (const auto& [filePath, entry] : index.getEntries()) {
        if (!fs::exists(filePath)) continue;

        totalFiles++;
        std::cout << "Syncing: " << filePath << " ... ";
        auto result = syncEngine.syncFileWithRemoteCAS(filePath, *remote);

        if (result.success) {
            totalUploadedChunks += result.uploaded_chunks;
            totalSkippedChunks += result.skipped_chunks;
            totalBytesUploaded += result.bytes_uploaded;
            totalBytesSaved += result.bytes_saved_by_dedup;
            std::cout << "[OK] (" << result.uploaded_chunks << " uploaded, "
                      << result.skipped_chunks << " deduped - "
                      << std::fixed << std::setprecision(1) << result.deduplication_ratio << "% saved)\n";
        } else {
            std::cout << "[FAILED: " << result.error_message << "]\n";
        }
    }

    std::cout << "\n=======================================================\n";
    std::cout << "  AI-GIT CAS PUSH SUMMARY\n";
    std::cout << "  Files Synced:        " << totalFiles << "\n";
    std::cout << "  Chunks Uploaded:     " << totalUploadedChunks << " (" << std::fixed << std::setprecision(2) << (totalBytesUploaded / (1024.0 * 1024.0)) << " MB)\n";
    std::cout << "  Chunks Deduplicated: " << totalSkippedChunks << " (" << std::fixed << std::setprecision(2) << (totalBytesSaved / (1024.0 * 1024.0)) << " MB)\n";
    std::cout << "=======================================================\n";
    return true;
}

} // namespace Commands