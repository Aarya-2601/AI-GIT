#ifndef BLOB_HPP
#define BLOB_HPP

#include <models/object.hpp>

namespace Models{
    class Blob: public GitObject{  //inherits from the GitObject class
        private:
            std::string mcontent;
        
        public:
            Blob()=default;  //default construcor

            //initialize directly with file string contents
            explicit Blob(std::string content);  //parameterized constructor
            std::string getTypeString() const override{  //overrride=perfectly matches the layout of the virtual class
                return "blob";
            }

            //pack raw data with the blob[size]/0 header
            std::string Models::Blob::serialize() const override;

            const std::string& getContent() const{ 
                return mcontent; 
            }
    };
}

#endif