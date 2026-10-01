#include "../sync/cas_sync_engine.hpp"
#include "../hashing/hashing.hpp"
#include "../inspection/file_inspector.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace Core::Sync;

void printUsage() {
    std::cout << "Usage: aigit-sync <command> [args...]\n\n"
              << "Commands:\n"
              << "  push <file_path> <remote_cas_uri>\n"
              << "      Uploads file using format-aware CAS sync with zero-redundancy delta negotiation.\n"
              << "      remote_cas_uri can be a local directory path, 'cas://<path>', or 'http://...'.\n\n"
              << "  pull <manifest_id> <output_path> <remote_cas_uri>\n"
              << "      Downloads chunks from remote CAS and reconstructs bit-exact file.\n\n"
              << "  status <file_path> <remote_cas_uri>\n"
              << "      Dry-run delta query: checks chunk presence on remote CAS.\n\n"
              << "  cat-manifest <manifest_id> <remote_cas_uri>\n"
              << "      Prints the remote CAS manifest JSON.\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "push") {
        if (argc < 4) {
            std::cerr << "Error: 'push' requires <file_path> and <remote_cas_uri>\n";
            return 1;
        }
        std::string filePath = argv[2];
        std::string remoteUri = argv[3];

        auto remote = createCASRemote(remoteUri);
        CASSyncEngine engine;

        SyncResult res = engine.syncFileWithRemoteCAS(filePath, *remote);
        std::cout << res.summaryJson() << std::endl;
        return res.success ? 0 : 2;

    } else if (command == "pull") {
        if (argc < 5) {
            std::cerr << "Error: 'pull' requires <manifest_id>, <output_path>, and <remote_cas_uri>\n";
            return 1;
        }
        std::string manifestId = argv[2];
        std::string outputPath = argv[3];
        std::string remoteUri = argv[4];

        auto remote = createCASRemote(remoteUri);
        CASSyncEngine engine;

        bool ok = engine.reconstructFileFromRemoteCAS(manifestId, outputPath, *remote);
        if (ok) {
            std::cout << "{\n"
                      << "  \"status\": \"SUCCESS\",\n"
                      << "  \"manifest_id\": \"" << manifestId << "\",\n"
                      << "  \"output_path\": \"" << outputPath << "\"\n"
                      << "}\n";
            return 0;
        } else {
            std::cout << "{\n"
                      << "  \"status\": \"FAILED\",\n"
                      << "  \"manifest_id\": \"" << manifestId << "\",\n"
                      << "  \"error\": \"Reconstruction failed or integrity check mismatch\"\n"
                      << "}\n";
            return 2;
        }

    } else if (command == "cat-manifest") {
        if (argc < 4) {
            std::cerr << "Error: 'cat-manifest' requires <manifest_id> and <remote_cas_uri>\n";
            return 1;
        }
        std::string manifestId = argv[2];
        std::string remoteUri = argv[3];

        auto remote = createCASRemote(remoteUri);
        std::string manifestJson;
        if (remote->getManifest(manifestId, manifestJson)) {
            std::cout << manifestJson << std::endl;
            return 0;
        } else {
            std::cerr << "Manifest not found: " << manifestId << std::endl;
            return 2;
        }

    } else if (command == "status") {
        if (argc < 4) {
            std::cerr << "Error: 'status' requires <file_path> and <remote_cas_uri>\n";
            return 1;
        }
        std::string filePath = argv[2];
        std::string remoteUri = argv[3];

        auto remote = createCASRemote(remoteUri);
        auto inspection = Core::FileInspector::inspect(filePath);
        std::cout << "{\n"
                  << "  \"file\": \"" << filePath << "\",\n"
                  << "  \"format\": \"" << Core::FileInspector::formatToString(inspection.format) << "\",\n"
                  << "  \"remote\": \"" << remote->getRemoteUri() << "\",\n"
                  << "  \"remote_type\": \"" << remote->getRemoteType() << "\"\n"
                  << "}\n";
        return 0;

    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        printUsage();
        return 1;
    }
}
