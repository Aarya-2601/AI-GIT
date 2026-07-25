#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>

namespace Core{ 
    class Config{
        private:
        std::map<std::string, std::string> settings;

        public:
        Config()=default;
        void load(const std::string& configPath=".aigit/config");
        void save(const std::string& configPath=".aigit/config") const;
        void set(const std::string& key, const std::string& value);
        std::string get(const std::string& key, const std::string& defaultValue="") const;

        std::string getAuthorName() const;
        std::string getAuthorEmail() const;
    };
}

#endif