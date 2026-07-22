#ifndef INDEX_HPP
#define INDEX_HPP

#include <string>
#include <map>

namespace Core{
    struct IndexEntry{
        std::string path;
        std::string hash;
        std::string mode;  //directory/file code
    };

    class Index{
        private:
        std::map<std::string, IndexEntry> entries;

        public:
        Index()=default;
        void load(const std::string& Indexpath=".aigit/index");
        void save(const std::string& Indexpath=".aigit/index") const;

        void addEntry(const IndexEntry& entry);
        void removeEntry(const std::string& path);

        const std::map<std::string, IndexEntry>& getEntries() const{
            return entries;
        }
    };
}

#endif