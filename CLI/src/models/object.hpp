#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <string>

namespace Models{
<<<<<<< HEAD
    struct objectType
    {
        inline static const std::string blob= "blob";
=======
    struct objectType{
        inline static const std::string blob= "blob"; //const=read only, static=belongs to the struct rather than an object of the struct, inline=
        //allow this member to be a part of this header lowk
>>>>>>> aaryascrazycommits
        inline static const std::string tree= "tree";
        inline static const std::string commit= "commit";
    };

    class GitObject
    {
        public:
<<<<<<< HEAD
        virtual ~GitObject()= default; //virtual destructor, default implementation
=======
        virtual ~GitObject()= default;  //virtual class that cannot be called on its own using an obj's property
>>>>>>> aaryascrazycommits
        virtual std::string getTypeString() const= 0;
        virtual std::string serialize() const= 0;
    };
}

#endif  