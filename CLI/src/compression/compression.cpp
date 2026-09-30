#include "compression.hpp"
#include <zlib.h>
#include <cstring>
#include <vector>
#include <stdexcept>

namespace Core
{
    std::string compressString(const std::string& data)
    {
        z_stream zs;
        std::memset(&zs, 0, sizeof(zs));

        if (deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK)
        {
            throw std::runtime_error("Failed to initialize zlib deflate stream.");
        }

        zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
        zs.avail_in = static_cast<uInt>(data.size());

        int ret;
        uLongf maxCompressedSize = deflateBound(&zs, data.size());
        std::vector<char> outBuffer(maxCompressedSize);

        zs.next_out = reinterpret_cast<Bytef*>(outBuffer.data());
        zs.avail_out = static_cast<uInt>(outBuffer.size());

        ret = deflate(&zs, Z_FINISH);
        deflateEnd(&zs);

        if (ret != Z_STREAM_END)
        {
            throw std::runtime_error("An error occurred while deflating data payload.");
        }

        return std::string(outBuffer.data(), zs.total_out);
    }

    std::string decompressData(const std::string& compressedData)
    {
        z_stream zs;
        std::memset(&zs, 0, sizeof(zs));

        if (inflateInit(&zs) != Z_OK)
        {
            return "";
        }

        zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressedData.data()));
        zs.avail_in = static_cast<uInt>(compressedData.size());

        int ret;
        char chunkBuffer[32768];
        std::string uncompressedResult;

        do
        {
            zs.next_out = reinterpret_cast<Bytef*>(chunkBuffer);
            zs.avail_out = sizeof(chunkBuffer);

            ret = inflate(&zs, Z_NO_FLUSH);

            if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR)
            {
                inflateEnd(&zs);
                return "";
            }

            int bytesUnpacked = sizeof(chunkBuffer) - zs.avail_out;
            uncompressedResult.append(chunkBuffer, bytesUnpacked);

        } while (ret != Z_STREAM_END);

        inflateEnd(&zs);
        return uncompressedResult;
    }
}
