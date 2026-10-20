#include "add.hpp"
#include "../core/filesystem.hpp"
#include "../core/hashing.hpp"
#include "../core/storage.hpp"
#include "../core/compression.hpp"
#include "../core/index.hpp"
#include "../core/mmap_file.hpp"
#include "../inspection/file_inspector.hpp"
#include "../chunking/tensor_fastcdc.hpp"
#include "../hashing/xxhash_engine.hpp"
#include "../compression/bitshuffle_zstd.hpp"
#include "../diffing/metadata_harvester.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

namespace Commands
{   
    static std::string normalizePath(const fs::path& p){
        std::string pathStr = p.generic_string();
        if(pathStr.rfind("./", 0) == 0){
            pathStr = pathStr.substr(2);
        }
        return pathStr;
    }

    static bool processfile(const fs::path& filePath, Core::Index& indexEntries){
        std::string normPath = normalizePath(filePath);

        if(normPath.find(".aigit") != std::string::npos || 
           normPath.find("build/") != std::string::npos || 
           normPath.find(".git") != std::string::npos || 
           normPath.find(".vscode/") != std::string::npos || 
           normPath.find("vcpkg/") != std::string::npos){
            return true;
        }

        uintmax_t fSize = 0;
        try {
            fSize = fs::file_size(filePath);
        } catch (...) {
            return false;
        }

        // Fast path for Large Model Files (>= 1MB) using Zero-Copy MMap & Tensor FastCDC
        if (fSize >= 1024 * 1024) {
            Core::MMapFile mmap;
            if (!mmap.open(filePath, true)) {
                std::cerr << "Error: Could not mmap large file: " << normPath << std::endl;
                return false;
            }

            auto insp = Core::FileInspector::inspectBuffer(mmap.data(), std::min(mmap.size(), size_t(64)), mmap.size(), &filePath);
            auto chunks = Core::Chunking::TensorAwareFastCDC::chunkMappedFile(mmap.data(), mmap.size(), filePath);

            std::vector<Core::Hashing::Hash128> leaves;
            for (const auto& c : chunks) {
                leaves.push_back(c.xxhash128);

                // Write compressed chunk to CAS store if not already present
                if (!fs::exists(Core::Storage::getObjectPath(c.sha256))) {
                    std::vector<uint8_t> compressed;
                    if (c.dtype == "F32" || c.dtype == "F16" || c.dtype == "BF16") {
                        compressed = Core::Compression::BitshuffleCodec::compressFloatWeights(mmap.data() + c.offset, c.length);
                    } else {
                        std::string rawStr(reinterpret_cast<const char*>(mmap.data() + c.offset), c.length);
                        std::string z = Core::compressString(rawStr);
                        compressed.assign(z.begin(), z.end());
                    }
                    std::string compStr(reinterpret_cast<const char*>(compressed.data()), compressed.size());
                    Core::Storage::writeObject(c.sha256, compStr);
                }
            }

            std::string merkleRoot = Core::Hashing::XXHashEngine::buildMerkleTreeRootSHA256(leaves);

            // Harvest tensor layer metadata into SQLite if available
            if (insp.format == Core::FileFormat::SafeTensors) {
                sqlite3* db = nullptr;
                if (sqlite3_open(".aigit/metadata.db", &db) == SQLITE_OK) {
                    Core::ModelMetadata meta;
                    if (Core::MetadataHarvester::harvestSafeTensorsMetadata(filePath, meta)) {
                        Core::MetadataHarvester::persistModelToSQLite(db, merkleRoot, meta);
                    }
                    sqlite3_close(db);
                }
            }

            const auto& existingEntries = indexEntries.getEntries();
            auto it = existingEntries.find(normPath);
            if (it != existingEntries.end() && it->second.hash == merkleRoot) {
                return true;
            }

            indexEntries.addEntry(Core::IndexEntry(normPath, merkleRoot, "100644"));
            std::cout << "Added " << normPath << " (" << std::fixed << std::setprecision(2)
                      << (fSize / (1024.0 * 1024.0)) << " MB " << Core::FileInspector::formatToString(insp.format)
                      << ", " << chunks.size() << " chunks, Merkle root: " << merkleRoot.substr(0, 12) << "...) to staging area." << std::endl;
            return true;
        }

        // Standard path for small regular files (< 1MB)
        std::ifstream inFile(filePath, std::ios::binary);
        if(!inFile.is_open()){
            std::cerr << "Error: Could not open file: " << filePath << std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer << inFile.rdbuf();
        std::string fileContent = buffer.str();
        inFile.close();

        Models::Blob blobObject(fileContent);
        std::string storePayload = blobObject.serialize();
        std::string sha256Hash = Core::calcSHA256(storePayload);
        if(sha256Hash.empty()){
            std::cerr << "Error: Cryptographic hashing mechanism failed." << std::endl;
            return false;
        }

        const auto& existingEntries = indexEntries.getEntries();
        auto it = existingEntries.find(normPath);
        if(it != existingEntries.end() && it->second.hash == sha256Hash){
            return true;
        }

        try{
            std::string compressedData = Core::compressString(storePayload);
            if(!Core::Storage::writeObject(sha256Hash, compressedData)){
                std::cerr << "Error: Storage system failed to write object blob." << std::endl;
                return false;
            }
        }
        catch(const std::exception& e){
            std::cerr << "Compression/Storage Error: " << e.what() << std::endl;
            return false;
        }

        indexEntries.addEntry(Core::IndexEntry(normPath, sha256Hash, "100644"));
        std::cout << "Added " << normPath << " to staging area." << std::endl;
        return true;
    }

    int runAdd(const std::vector<std::string>& targets)
    {
        if (!fs::exists(".aigit"))
        {
            std::cerr << "Error: Not an AI-Git repository." << std::endl;
            return 1;
        }
        if(targets.empty()){
            std::cerr << "Nothing specified, nothing added." << std::endl;
            return 0;
        }
        
        Core::Index indexEntries;
        indexEntries.load(".aigit/index");
        
        for(const auto& target : targets){
            fs::path targetPath(target);
            if(!fs::exists(targetPath)){
                std::cerr << "Error: Path does not exist: " << target << std::endl;
                continue;
            }
            if(fs::is_directory(targetPath)){
                for(const auto& entry : fs::recursive_directory_iterator(targetPath)){
                    std::string pStr = entry.path().generic_string();

                    if(pStr.find(".aigit") != std::string::npos || pStr.find("build/") != std::string::npos || pStr.find(".git") != std::string::npos || pStr.find(".vscode/") != std::string::npos || pStr.find("vcpkg/") != std::string::npos ){
                        continue;
                    }
                    if(fs::is_regular_file(entry.status())){
                        processfile(entry.path(), indexEntries);
                    }
                }
            }
            else if (fs::is_regular_file(targetPath)){
                processfile(targetPath, indexEntries);
            }
            else{
                std::cerr << "Warning: Skipping unsupported path: " << target << std::endl;
            }
        }

        indexEntries.save(".aigit/index");
        return 0;
    }

    int runAdd(const std::string& filePath){
        return runAdd(std::vector<std::string>{filePath});
    }
}