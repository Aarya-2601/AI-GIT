#include "hashing.hpp" //this includes header file we have defined
#include <openssl/evp.h>  //this includes libraries pre-defined, evp is the envelope API
#include <sstream>  //in-memory stream used to assemble strings cleanly
#include <iomanip>  //used for input and output manipulation used in setfill and setw

using namespace std;

namespace Core{
    string calcSHA256(const string& content){
        //openssl message digest context struct allocated heap in memory
        EVP_MD_CTX* context = EVP_MD_CTX_new();
        //if ran out of memory and could not allocate context
        if (!context) return "";

        //allocated fixed size bite array on the stack to hold raw binary output, hash is tha array
        unsigned char hash[EVP_MAX_MD_SIZE];
        //number of bytes written to hash, 32 for SHA-256
        unsigned int length=0;
        
                                   //algorithm descriptor btw
        EVP_DigestInit_ex(context, EVP_sha256(), nullptr);  //tells the dispatcher which algo we want to use, and the provider
        EVP_DigestUpdate(context, content.c_str(), content.size());  //c_str() gives pointer to the array
        EVP_DigestFinal_ex(context, hash, &length);//expects pointer to an unsigned int so length is unsigned int
        //performs padding, writes 32 bytes to hash array
        EVP_MD_CTX_free(context);

        std::stringstream ss;
        for(unsigned int i=0; i<length; ++i){
            ss<<hex<<setfill('0')<<setw(2)<<static_cast<int>(hash[i]);
            //formats the number as base 16 hexadecimal, pads single digit values with 0, formats to integer from unsigned char
        }
        return ss.str();
    }
}