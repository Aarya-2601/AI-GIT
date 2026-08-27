#include "chunking.hpp"
#include "hashing.hpp"
#include <fstream>
#include <algorithm>
#include <stdexcept>

// 256 64-bit random values for the Gear hashing matrix
static const uint64_t GEAR_MATRIX[256] = {
    0x3565a0ec7b80a563ULL, 0x1f0b094602f7ff60ULL, 0xa1bf785420352ef2ULL, 0xd008dd52f75a6c38ULL,
    0x2c68e4209930f555ULL, 0x4896eec50e685f0aULL, 0x6e9f783ef2e987c9ULL, 0x93309a473f1d8c11ULL,
    0x442dfa8f108849adULL, 0xa8c49e7b233a5987ULL, 0xcb953a7629b35bc4ULL, 0xd9b7754668b57732ULL,
    0x67db2b56e6d338f0ULL, 0x92f9d51a660a92d7ULL, 0x199fa688f1521908ULL, 0x51c36b4474776b38ULL,
    0x8e8a93e2dc11cf9aULL, 0x53503db458efda6eULL, 0xb8beec072c4ec096ULL, 0x446d3211e4e69d72ULL,
    0x88981f440b82f64dULL, 0x2287955c2763f0aeULL, 0xdf8435d8cfd98caeULL, 0x9a805f6cb1cfd8c7ULL,
    0x88498f48b12f6a7cULL, 0x1d9539268f7b5a88ULL, 0x4e6b2c68e4209930ULL, 0x2649a8c49e7b233aULL,
    0x92e5cb953a7629b3ULL, 0x76b5d9b7754668b5ULL, 0x82f967db2b56e6d3ULL, 0x56a692f9d51a660aULL,
    0x4811199fa688f152ULL, 0x9f5151c36b447477ULL, 0xb0e88e8a93e2dc11ULL, 0xf05353503db458efULL,
    0xc6b8b8beec072c4eULL, 0x9844446d3211e4e6ULL, 0x828888981f440b82ULL, 0x63222287955c2763ULL,
    0xd9dfdf8435d8cfd9ULL, 0xcb9a9a805f6cb1cfULL, 0x2f8888498f48b12fULL, 0x7b1d1d9539268f7bULL,
    0x204e4e6b2c68e420ULL, 0x7b262649a8c49e7bULL, 0x769292e5cb953a76ULL, 0x467676b5d9b77546ULL,
    0x568282f967db2b56ULL, 0x1a5656a692f9d51aULL, 0x88484811199fa688ULL, 0x449f9f5151c36b44ULL,
    0xe2b0b0e88e8a93e2ULL, 0xb4f0f05353503db4ULL, 0x07c6c6b8b8beec07ULL, 0x11989844446d3211ULL,
    0x4482828888981f44ULL, 0x5c6363222287955cULL, 0xd8d9d9dfdf8435d8ULL, 0x6ccb9a9a805f6cb1ULL,
    0x8f2f88888498f48bULL, 0x397b1d1d9539268fULL, 0x68204e4e6b2c68e4ULL, 0x9e7b262649a8c49eULL,
    0x3a769292e5cb953aULL, 0x75467676b5d9b775ULL, 0x2b568282f967db2bULL, 0xd51a5656a692f9d5ULL,
    0xa688484811199fa6ULL, 0x6b449f9f5151c36bULL, 0x93e2b0b0e88e8a93ULL, 0x3db4f0f05353503dULL,
    0xec07c6c6b8b8beecULL, 0x3211989844446d32ULL, 0x1f4482828888981fULL, 0x955c636322228795ULL,
    0x35d8d9d9df8435d8ULL, 0x5f6ccb9a9a805f6cULL, 0x8f48b12f6a7c8849ULL, 0x39268f7b5a881d95ULL,
    0x2c68e42099304e6bULL, 0x9e7b233a2649a8c4ULL, 0x3a7629b392e5cb95ULL, 0x754668b576b5d9b7ULL,
    0x2b56e6d382f967dbULL, 0xd51a660a56a692f9ULL, 0xa688f1524811199fULL, 0x6b4474779f5151c3ULL,
    0x93e2dc11b0e88e8aULL, 0x3db458eff0535350ULL, 0xec072c4ec6b8b8beULL, 0x3211e4e69844446dULL,
    0x1f440b8282888898ULL, 0x955c276363222287ULL, 0x35d8cfd9d9dfdf84ULL, 0x5f6cb1cfcb9a9a80ULL,
    0x8f48b12f8888498fULL, 0x39268f7b1d1d9539ULL, 0x2c68e4204e4e6b2cULL, 0x9e7b233a7b262649ULL,
    0x3a7629b3769292e5ULL, 0x754668b5467676b5ULL, 0x2b56e6d3568282f9ULL, 0xd51a660a1a5656a6ULL,
    0xa688f15288484811ULL, 0x6b447477449f9f51ULL, 0x93e2dc11e2b0b0e8ULL, 0x3db458efb4f0f053ULL,
    0xec072c4e07c6c6b8ULL, 0x3211e4e611989844ULL, 0x1f440b8244828288ULL, 0x955c27635c636322ULL,
    0x35d8cfd9d8d9d9dfULL, 0x5f6cb1cf6ccb9a9aULL, 0x88498f48b12f6a7cULL, 0x1d9539268f7b5a88ULL,
    0x4e6b2c68e4209930ULL, 0x2649a8c49e7b233aULL, 0x92e5cb953a7629b3ULL, 0x76b5d9b7754668b5ULL,
    0x82f967db2b56e6d3ULL, 0x56a692f9d51a660aULL, 0x4811199fa688f152ULL, 0x9f5151c36b447477ULL,
    0xb0e88e8a93e2dc11ULL, 0xf05353503db458efULL, 0xc6b8b8beec072c4eULL, 0x9844446d3211e4e6ULL,
    0x828888981f440b82ULL, 0x63222287955c2763ULL, 0xd9dfdf8435d8cfd9ULL, 0xcb9a9a805f6cb1cfULL,
    0x2f8888498f48b12fULL, 0x7b1d1d9539268f7bULL, 0x204e4e6b2c68e420ULL, 0x7b262649a8c49e7bULL,
    0x769292e5cb953a76ULL, 0x467676b5d9b77546ULL, 0x568282f967db2b56ULL, 0x1a5656a692f9d51aULL,
    0x88484811199fa688ULL, 0x449f9f5151c36b44ULL, 0xe2b0b0e88e8a93e2ULL, 0xb4f0f05353503db4ULL,
    0x07c6c6b8b8beec07ULL, 0x11989844446d3211ULL, 0x4482828888981f44ULL, 0x5c6363222287955cULL,
    0xd8d9d9dfdf8435d8ULL, 0x6ccb9a9a805f6cb1ULL, 0x8f2f88888498f48bULL, 0x397b1d1d9539268fULL,
    0x68204e4e6b2c68e4ULL, 0x9e7b262649a8c49eULL, 0x3a769292e5cb953aULL, 0x75467676b5d9b775ULL,
    0x2b568282f967db2bULL, 0xd51a5656a692f9d5ULL, 0xa688484811199fa6ULL, 0x6b449f9f5151c36bULL,
    0x93e2b0b0e88e8a93ULL, 0x3db4f0f05353503dULL, 0xec07c6c6b8b8beecULL, 0x3211989844446d32ULL,
    0x1f4482828888981fULL, 0x955c636322228795ULL, 0x35d8d9d9df8435d8ULL, 0x5f6ccb9a9a805f6cULL,
    0x8f48b12f6a7c8849ULL, 0x39268f7b5a881d95ULL, 0x2c68e42099304e6bULL, 0x9e7b233a2649a8c4ULL,
    0x3a7629b392e5cb95ULL, 0x754668b576b5d9b7ULL, 0x2b56e6d382f967dbULL, 0xd51a660a56a692f9ULL,
    0xa688f1524811199fULL, 0x6b4474779f5151c3ULL, 0x93e2dc11b0e88e8aULL, 0x3db458eff0535350ULL,
    0xec072c4ec6b8b8beULL, 0x3211e4e69844446dULL, 0x1f440b8282888898ULL, 0x955c276363222287ULL,
    0x35d8cfd9d9dfdf84ULL, 0x5f6cb1cfcb9a9a80ULL, 0x8f48b12f8888498fULL, 0x39268f7b1d1d9539ULL,
    0x2c68e4204e4e6b2cULL, 0x9e7b233a7b262649ULL, 0x3a7629b3769292e5ULL, 0x754668b5467676b5ULL,
    0x2b56e6d3568282f9ULL, 0xd51a660a1a5656a6ULL, 0xa688f15288484811ULL, 0x6b447477449f9f51ULL,
    0x93e2dc11e2b0b0e8ULL, 0x3db458efb4f0f053ULL, 0xec072c4e07c6c6b8ULL, 0x3211e4e611989844ULL,
    0x1f440b8244828288ULL, 0x955c27635c636322ULL, 0x35d8cfd9d8d9d9dfULL, 0x5f6cb1cf6ccb9a9aULL,
    0x88498f48b12f6a7cULL, 0x1d9539268f7b5a88ULL, 0x4e6b2c68e4209930ULL, 0x2649a8c49e7b233aULL,
    0x92e5cb953a7629b3ULL, 0x76b5d9b7754668b5ULL, 0x82f967db2b56e6d3ULL, 0x56a692f9d51a660aULL,
    0x4811199fa688f152ULL, 0x9f5151c36b447477ULL, 0xb0e88e8a93e2dc11ULL, 0xf05353503db458efULL,
    0xc6b8b8beec072c4eULL, 0x9844446d3211e4e6ULL, 0x828888981f440b82ULL, 0x63222287955c2763ULL,
    0xd9dfdf8435d8cfd9ULL, 0xcb9a9a805f6cb1cfULL, 0x2f8888498f48b12fULL, 0x7b1d1d9539268f7bULL,
    0x204e4e6b2c68e420ULL, 0x7b262649a8c49e7bULL, 0x769292e5cb953a76ULL, 0x467676b5d9b77546ULL,
    0x568282f967db2b56ULL, 0x1a5656a692f9d51aULL, 0x88484811199fa688ULL, 0x449f9f5151c36b44ULL,
    0xe2b0b0e88e8a93e2ULL, 0xb4f0f05353503db4ULL, 0x07c6c6b8b8beec07ULL, 0x11989844446d3211ULL,
    0x4482828888981f44ULL, 0x5c6363222287955cULL, 0xd8d9d9dfdf8435d8ULL, 0x6ccb9a9a805f6cb1ULL
};

std::vector<Chunk> Chunking::chunkBuffer(
    const uint8_t* data,
    size_t size
)
{
    std::vector<Chunk> chunks;

    if (data == nullptr || size == 0)
    {
        return chunks;
    }

    size_t cursor = 0;

    while (cursor < size)
    {
        size_t remaining =
            size - cursor;

        if (remaining <= MIN_SIZE)
        {
            std::string chunkData(
                reinterpret_cast<const char*>(
                    data + cursor
                ),
                remaining
            );

            std::string hash =
                Core::calcSHA256(chunkData);

            chunks.push_back(
                {
                    cursor,
                    remaining,
                    hash
                }
            );

            break;
        }

        size_t maxChunk =
            std::min(
                remaining,
                MAX_SIZE
            );

        size_t maxPosition =
            cursor + maxChunk;

        size_t midPosition =
            cursor +
            std::min(
                remaining,
                AVG_SIZE
            );

        size_t searchPosition =
            cursor + MIN_SIZE;

        size_t splitPoint =
            maxPosition;

        uint64_t fingerprint = 0;

        while (
            searchPosition < midPosition
        )
        {
            fingerprint =
                (fingerprint << 1) +
                GEAR_MATRIX[
                    data[searchPosition]
                ];

            if (
                (fingerprint & MASK_S)
                == 0
            )
            {
                splitPoint =
                    searchPosition + 1;

                break;
            }

            ++searchPosition;
        }

        if (splitPoint == maxPosition)
        {
            while (
                searchPosition < maxPosition
            )
            {
                fingerprint =
                    (fingerprint << 1) +
                    GEAR_MATRIX[
                        data[searchPosition]
                    ];

                if (
                    (fingerprint & MASK_L)
                    == 0
                )
                {
                    splitPoint =
                        searchPosition + 1;

                    break;
                }

                ++searchPosition;
            }
        }

        size_t chunkLength =
            splitPoint - cursor;

        std::string chunkData(
            reinterpret_cast<const char*>(
                data + cursor
            ),
            chunkLength
        );

        std::string chunkHash =
            Core::calcSHA256(
                chunkData
            );

        chunks.push_back(
            {
                cursor,
                chunkLength,
                chunkHash
            }
        );

        cursor = splitPoint;
    }

    return chunks;
}

std::vector<Chunk> Chunking::chunkFile(
    const std::string& filePath
)
{
    std::ifstream file(
        filePath,
        std::ios::binary
    );

    if (!file.is_open())
    {
        throw std::runtime_error(
            "Failed to open file for chunking: " +
            filePath
        );
    }

    std::vector<Chunk> chunks;

    size_t globalOffset = 0;

    while (true)
    {
        std::vector<uint8_t> buffer(MAX_SIZE);

        file.read(
            reinterpret_cast<char*>(
                buffer.data()
            ),
            static_cast<std::streamsize>(
                MAX_SIZE
            )
        );

        size_t bytesRead =
            static_cast<size_t>(
                file.gcount()
            );

        if (bytesRead == 0)
        {
            break;
        }

        buffer.resize(bytesRead);

        size_t searchPosition =
            std::min(
                MIN_SIZE,
                bytesRead
            );

        size_t splitPoint =
            bytesRead;

        uint64_t fingerprint = 0;

        if (bytesRead > MIN_SIZE)
        {
            size_t midPosition =
                std::min(
                    AVG_SIZE,
                    bytesRead
                );

            while (
                searchPosition < midPosition
            )
            {
                fingerprint =
                    (fingerprint << 1) +
                    GEAR_MATRIX[
                        buffer[searchPosition]
                    ];

                if (
                    (fingerprint & MASK_S)
                    == 0
                )
                {
                    splitPoint =
                        searchPosition + 1;

                    break;
                }

                ++searchPosition;
            }

            if (
                splitPoint == bytesRead
            )
            {
                while (
                    searchPosition < bytesRead
                )
                {
                    fingerprint =
                        (fingerprint << 1) +
                        GEAR_MATRIX[
                            buffer[searchPosition]
                        ];

                    if (
                        (fingerprint & MASK_L)
                        == 0
                    )
                    {
                        splitPoint =
                            searchPosition + 1;

                        break;
                    }

                    ++searchPosition;
                }
            }
        }

        std::string chunkData(
            reinterpret_cast<const char*>(
                buffer.data()
            ),
            splitPoint
        );

        std::string chunkHash =
            Core::calcSHA256(
                chunkData
            );

        chunks.push_back(
            {
                globalOffset,
                splitPoint,
                chunkHash
            }
        );

        globalOffset +=
            splitPoint;

        if (splitPoint < bytesRead)
        {
            std::streamoff unusedBytes =
                static_cast<std::streamoff>(
                    bytesRead - splitPoint
                );

            file.clear();

            file.seekg(
                -unusedBytes,
                std::ios::cur
            );

            if (!file)
            {
                throw std::runtime_error(
                    "Failed to reposition file while chunking."
                );
            }
        }

        if (
            bytesRead < MAX_SIZE &&
            splitPoint == bytesRead
        )
        {
            break;
        }
    }

    return chunks;
}
