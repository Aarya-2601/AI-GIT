#include "core/compression.hpp"
#include <zlib.h>
#include <cstring>  //low level mem manipulation
#include <vector>
#include <stdexcept>  //standard exceptions library

namespace Core{

    //compresses block to small string
    std::string compressString(const std::string& data){
        //zstream is a struct of zlib
        z_stream zs;
        //set all the mem blocks to zero manually
        std::memset(&zs, 0, sizeof(zs));

        //if it fails to get ready for compression throw error
        if(deflateInit(&zs, Z_DEFAULT_COMPRESSION)!= Z_OK){
            throw std::runtime_error("Failed to initialize zlib deflate stream.");
        }

        //temporarily strips away the read only instruction and forces the bytes to convert to zlib type bytef
        zs.next_in=reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
        //number of bytes in the input
        zs.avail_in=static_cast<uInt>(data.size());

        int ret;
        //if the data is incompressible how much size it will take, allocate a vector
        uLongf maxCompressedSize=deflateBound(&zs, data.size());
        std::vector<char> outBuffer(maxCompressedSize);

        //point to the start of the vector and mt of blank space to write to
        zs.next_out=reinterpret_cast<Bytef*>(outBuffer.data());
        zs.avail_out=static_cast<uInt>(outBuffer.size());

        //run deflate op
        ret=deflate(&zs, Z_FINISH);
        
        //cleanup internal zlib allocations
        deflateEnd(&zs);

        //if does not give zstreamend then throw error
        if(ret!= Z_STREAM_END){
            throw std::runtime_error("An error occurred while deflating data payload.");
        }

        //return chunk as string
        return std::string(outBuffer.data(), zs.total_out);
    }


    //decompression
    std::string decompressData(const std::string& compressedData){
        z_stream zs;
        //set mem to 0
        std::memset(&zs, 0, sizeof(zs));

        //if not proper restoration, error
        if(inflateInit(&zs)!= Z_OK){
            return "";
        }

        zs.next_in=reinterpret_cast<Bytef*>(const_cast<char*>(compressedData.data()));
        zs.avail_in=static_cast<uInt>(compressedData.size());

        int ret;
        char chunkBuffer[32768];  //size of chunk=32KB defined
        std::string uncompressedResult;

        //loop streaming data until the end is reached
        do{
            zs.next_out=reinterpret_cast<Bytef*>(chunkBuffer);
            zs.avail_out=sizeof(chunkBuffer);

            //unpack as much data as possible until 32KB is filled
            ret=inflate(&zs, Z_NO_FLUSH);

            if (ret==Z_NEED_DICT || ret==Z_DATA_ERROR || ret==Z_MEM_ERROR){
                inflateEnd(&zs);
                return "";
            }

            int bytesUnpacked=sizeof(chunkBuffer)-zs.avail_out;  //how much space actually taken by the comp string
            uncompressedResult.append(chunkBuffer, bytesUnpacked);  //put the decompressed bytes to the end of the chunk buffer

        }while (ret!= Z_STREAM_END);

        inflateEnd(&zs);
        return uncompressedResult;
    }
}