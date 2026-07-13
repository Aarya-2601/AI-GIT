#include "hashing.hpp" //this includes header file we have defined
#include <openssl/evp.h>  //this includes libraries pre-defined, evp is the envelope library
#include <sstream>  //
#include <iomanip>  //used for input and output manipulation used in setfill and setw

using namespace std;

namespace Core{
    string calcSHA256(const string& content) {
        EVP_MD_CTX* context = EVP_MD_CTX_new();
        if (!context) return "";

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int length = 0;

        EVP_DigestInit_ex(context, EVP_sha256(), nullptr);  //tells the dispatcher which algo we want to use
        EVP_DigestUpdate(context, content.c_str(), content.size());
        EVP_DigestFinal_ex(context, hash, &length);
        EVP_MD_CTX_free(context);

        std::stringstream ss;
        for (unsigned int i = 0; i < length; ++i) {
            ss << hex<< setfill('0') <<setw(2) << static_cast<int>(hash[i]);
        }
        return ss.str();
    }
}