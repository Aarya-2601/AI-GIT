#include "hash_object.hpp"

namespace Commands
{

    int runHashObject(const std::string& filePath)
    {
        //verify whether file exists on disk
        if (!fs::exists(filePath))
        {
            std::cerr<< "Error: File does not exist: "<< filePath<< std::endl;  //cerr=cout with error giving capabilities
            return 1;
        }
        //func returns boolean value false if it is a folder or a system file and not a regular file
        if (!fs::is_regular_file(filePath))
        {
            std::cerr<< "Error: Path specified is not a regular file: "<< filePath<< std::endl;
            return 1;
        }

        try
        {
            // Same raw-bytes / chunk-manifest hashing and storage path as
            // `add` (StorageManager::storeFile), so hash-object prints the
            // same ID `add` would record for identical content -- no
            // "blob <n>\0" wrapper.
            Storage::StorageManager storageManager(".aigit");
            std::string objectId = storageManager.storeFile(filePath);

            std::cout << objectId << std::endl;
            return 0;
        }
        catch(const std::exception& e)
        {
            std::cerr<< "Execution Exception: "<< e.what()<< std::endl;
            return 1;
        }
    }
}