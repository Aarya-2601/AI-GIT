#include "hashing.hpp"
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>

namespace Core
{
    std::string calcSHA256(const std::string& content)
    {
        EVP_MD_CTX* context = EVP_MD_CTX_new();
        if (!context) return "";

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int length = 0;

        EVP_DigestInit_ex(context, EVP_sha256(), nullptr);
        EVP_DigestUpdate(context, content.c_str(), content.size());
        EVP_DigestFinal_ex(context, hash, &length);
        EVP_MD_CTX_free(context);

        std::stringstream ss;
        for (unsigned int i = 0; i < length; ++i)
        {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(hash[i]);
        }
        return ss.str();
    }
}
