#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <string>

namespace Models{
    struct objectType{
        inline static const std::string blob= "blob";
        inline static const std::string tree= "tree";
        inline static const std::string commit= "commit";
    };

    class GitObject{
        public:
        virtual ~GitObject()= default;
        virtual std::string getTypeString() const= 0;
        virtual std::string serialize() const= 0;
    };
}

#endif