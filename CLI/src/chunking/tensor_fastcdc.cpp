#include "tensor_fastcdc.hpp"
#include "../hashing/hashing.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace Core::Chunking {

// Gear Hash lookup table for FastCDC rolling window
static const uint32_t GEAR_TABLE[256] = {
    0x8a970e66, 0xefb339f6, 0x191b2a5f, 0x3d78534f, 0x98d8d68a, 0xf5da6601, 0xd7c097a3, 0x4267b1d2,
    0x3472d1ec, 0x84f74a88, 0x431872f2, 0xa0f6b820, 0x36427861, 0x6a0496aa, 0xf6db4919, 0x9fa07a52,
    0x23f6e2fc, 0x851f5407, 0x546b5414, 0xe2224bc6, 0x37c9e1fa, 0x7ee63359, 0x345cfb34, 0x706830f6,
    0x74cc5b69, 0x76d1ff0b, 0xae257b62, 0x92ab18db, 0x1eb6663d, 0xb878396a, 0x524c5180, 0x533f3a61,
    0xa3e0613e, 0x6ec9ceb0, 0x77c4e71a, 0xd68c6a60, 0x56a0d08d, 0x67963e97, 0x11d08b60, 0x8870809d,
    0x389b2f63, 0xd9b87677, 0xd3642f0e, 0xd9f2c670, 0x64ec50c2, 0x482e4e70, 0xd903d334, 0x4045dd4d,
    0xe4c00360, 0x57344b30, 0x617114a9, 0xdbe0d661, 0xf3250884, 0xfeaf89fe, 0x9be8365e, 0xa4707d55,
    0xaf16408a, 0x5388cc82, 0xc0615535, 0x3f575685, 0xb0340269, 0xf69d1b72, 0xacaa5772, 0x0755220c,
    0x9b7f6074, 0x9b85cc8d, 0xb10852d6, 0x6007dd48, 0x6449e0ae, 0x6bf4f2ff, 0x7a6ae701, 0xd3877717,
    0x3bfcf96b, 0x42c976b3, 0xea3d166d, 0x7be49670, 0x541e3079, 0xc1472594, 0x67c275ab, 0x358b4a32,
    0x6129a26f, 0x52641f4d, 0x3cdc72d7, 0x4b74d693, 0x1fc34041, 0x118b3e40, 0x4f6424d1, 0xf4d670cc,
    0x53c07b5f, 0x327a3a39, 0x8236f5bc, 0xc7b47fe5, 0x46f7359d, 0x8d715264, 0x6a028081, 0xef74aef8,
    0x69ea2141, 0x1521b628, 0x2e007127, 0x9b7bae73, 0xb0f6e632, 0x98da7c16, 0x2b183303, 0x228ec512,
    0x78e18230, 0xbceb956f, 0x3f2e47e2, 0xd2be5d74, 0xf27da617, 0xc16a426d, 0xe5a83a16, 0x42f05d93,
    0xf86ec374, 0x86e18bc4, 0x2aec6034, 0x52d604e1, 0x3b5d4f47, 0x71686c3f, 0x46422efd, 0x670351a4,
    0x406082e6, 0x5e20d483, 0x89aff420, 0x45f6ac35, 0xcdb1d911, 0x03913d86, 0xb57b6a23, 0x27ec2244,
    0x45326020, 0x86184a60, 0x83e47856, 0x9a370e08, 0x403f1244, 0x65ca6352, 0xd02663cb, 0xedeed386,
    0x9fe37fe9, 0xd67a7402, 0xca0f32d9, 0xaf34cac6, 0x329ac1b0, 0x7971f269, 0xaa563d8b, 0x442642fe,
    0xeb1102f6, 0x89f229bc, 0x2719ddec, 0xacdd4557, 0x81a39872, 0x75e801e0, 0x3370f70b, 0x67678381,
    0x4a447d2d, 0xe5ff0e43, 0x0e2d0690, 0x2172f1a2, 0x0d4f43d4, 0xe9dc1a45, 0x282d60fa, 0x8d34fb10,
    0x6e734533, 0xa944f3ac, 0x644e8a73, 0x781614f0, 0xd7300a4e, 0xb77e9ab4, 0x9c0b7a5f, 0x82b49317,
    0xb53d4e65, 0xe91a4b89, 0x5be01f8f, 0xf611a624, 0x6af84266, 0x4af488c9, 0x80e64f84, 0x705d86e5,
    0xe50109c4, 0x082937a1, 0x450de3db, 0x18803b72, 0x70e61045, 0x67a71109, 0x71aa300f, 0x060e2610,
    0x23cc80e2, 0xcd444426, 0x07be0f13, 0x272e1d73, 0x29861964, 0x45a035ff, 0x6da88723, 0x40bc8322,
    0x4e9cb9f2, 0xa6dc38b1, 0x577258d4, 0x614272e2, 0x85fa4b73, 0xcfe042e6, 0x47e8100a, 0xa2c0e828,
    0xf5642ea9, 0xe0e035ec, 0x32a45a31, 0x60e44e54, 0xd3a5308c, 0x932c56b8, 0xd9e01080, 0x2462a0ee,
    0x610310ba, 0x8b44ff22, 0xe3450a82, 0x4222a4f6, 0x1a05bbcf, 0x5a55b20a, 0x97ff2d34, 0x5d0232e5,
    0x8204c4f8, 0x47efba25, 0xa4b19a16, 0xa452b4d2, 0xf7d1a51c, 0xd6e14fe6, 0x4316a4f0, 0x23aa40e8,
    0x360e2ce1, 0x24081e60, 0x8b61b4a0, 0xe9d45084, 0x54102040, 0x32204080, 0x7b008010, 0x22100800,
    0xd5aa4502, 0x48e42008, 0xa0840400, 0x42821004, 0x24080820, 0x9f040201, 0x40821000, 0xaba04104,
    0x20820820, 0x40820820, 0x20820820, 0x20820820, 0x10410410, 0x08208208, 0x04104104, 0x02082082,
    0x01041041, 0x80820820, 0x40410410, 0x20208208, 0x10104104, 0x08082082, 0x04041041, 0x02020820
};

std::vector<size_t> TensorAwareFastCDC::findFastCDCCutPoints(
    const uint8_t* data,
    size_t start,
    size_t length,
    const FastCDCProfile& profile
) {
    std::vector<size_t> cutPoints;
    if (length <= profile.minSize) {
        cutPoints.push_back(start + length);
        return cutPoints;
    }

    size_t curr = start;
    size_t end = start + length;

    while (curr < end) {
        if (end - curr <= profile.minSize) {
            cutPoints.push_back(end);
            break;
        }

        size_t minEnd = curr + profile.minSize;
        size_t maxEnd = std::min(curr + profile.maxSize, end);
        size_t avgPoint = curr + profile.avgSize;

        uint32_t fp = 0;
        size_t cut = maxEnd;

        // Phase 1: Small mask before avgPoint
        size_t p = minEnd;
        size_t limit1 = std::min(avgPoint, maxEnd);
        for (; p < limit1; ++p) {
            fp = (fp << 1) + GEAR_TABLE[data[p]];
            if ((fp & profile.maskS) == 0) {
                cut = p + 1;
                break;
            }
        }

        // Phase 2: Large mask after avgPoint
        if (cut == maxEnd && p < maxEnd) {
            for (; p < maxEnd; ++p) {
                fp = (fp << 1) + GEAR_TABLE[data[p]];
                if ((fp & profile.maskL) == 0) {
                    cut = p + 1;
                    break;
                }
            }
        }

        cutPoints.push_back(cut);
        curr = cut;
    }

    return cutPoints;
}

std::vector<TensorChunk> TensorAwareFastCDC::chunkMappedFile(
    const uint8_t* data,
    size_t size,
    const std::filesystem::path& filePath,
    const FastCDCProfile& profile
) {
    std::vector<TensorChunk> chunks;
    if (!data || size == 0) return chunks;

    auto inspection = Core::FileInspector::inspectBuffer(data, std::min(size, size_t(64)), size, &filePath);

    // Format-Aware SafeTensors Snapping:
    // Extract metadata header JSON length and tensor offsets
    if (inspection.format == FileFormat::SafeTensors && size > 16) {
        uint64_t headerLength = 0;
        std::memcpy(&headerLength, data, 8);

        if (headerLength > 0 && 8 + headerLength <= size) {
            size_t payloadOffset = 8 + static_cast<size_t>(headerLength);

            // Chunk 0: Header metadata chunk
            TensorChunk headerChunk;
            headerChunk.chunkIndex = 0;
            headerChunk.offset = 0;
            headerChunk.length = payloadOffset;
            headerChunk.tensorName = "__safetensors_header__";
            headerChunk.dtype = "JSON";
            headerChunk.isMetadata = true;
            headerChunk.xxhash128 = Hashing::XXHashEngine::hash128(data, headerChunk.length);
            headerChunk.sha256 = calcSHA256(std::string(reinterpret_cast<const char*>(data), headerChunk.length));
            chunks.push_back(headerChunk);

            // Parse tensor slices from JSON header
            std::string headerJson(reinterpret_cast<const char*>(data + 8), static_cast<size_t>(headerLength));
            
            // Extract tensor name and offsets: "data_offsets":[start, end]
            struct TensorSpan {
                std::string name;
                std::string dtype;
                size_t start;
                size_t end;
            };
            std::vector<TensorSpan> spans;

            size_t pos = 0;
            while ((pos = headerJson.find("\"data_offsets\"", pos)) != std::string::npos) {
                // Find tensor name preceding data_offsets
                size_t keyEnd = headerJson.rfind('"', pos - 1);
                size_t keyStart = (keyEnd != std::string::npos) ? headerJson.rfind('"', keyEnd - 1) : std::string::npos;
                std::string tName = "unknown_tensor";
                if (keyStart != std::string::npos && keyEnd != std::string::npos && keyEnd > keyStart) {
                    tName = headerJson.substr(keyStart + 1, keyEnd - keyStart - 1);
                }

                size_t arrStart = headerJson.find('[', pos);
                size_t arrEnd = headerJson.find(']', arrStart);
                if (arrStart != std::string::npos && arrEnd != std::string::npos) {
                    std::string nums = headerJson.substr(arrStart + 1, arrEnd - arrStart - 1);
                    size_t comma = nums.find(',');
                    if (comma != std::string::npos) {
                        size_t s = std::stoull(nums.substr(0, comma));
                        size_t e = std::stoull(nums.substr(comma + 1));
                        spans.push_back({ tName, "F32", s, e });
                    }
                }
                pos = arrEnd + 1;
            }

            // Sort tensor spans by start offset
            std::sort(spans.begin(), spans.end(), [](const TensorSpan& a, const TensorSpan& b) {
                return a.start < b.start;
            });

            // Boundary-aligned FastCDC for each tensor
            for (const auto& span : spans) {
                size_t tStart = payloadOffset + span.start;
                size_t tEnd = payloadOffset + span.end;
                if (tStart >= size || tEnd > size || tStart >= tEnd) continue;

                size_t tLen = tEnd - tStart;

                // Run FastCDC bounded STRICTLY within this tensor
                std::vector<size_t> cuts;
                if (tLen <= profile.avgSize) {
                    cuts.push_back(tEnd);
                } else {
                    cuts = findFastCDCCutPoints(data, tStart, tLen, profile);
                }
                size_t chunkStart = tStart;

                for (size_t cut : cuts) {
                    size_t chunkLen = cut - chunkStart;
                    TensorChunk tc;
                    tc.chunkIndex = chunks.size();
                    tc.offset = chunkStart;
                    tc.length = chunkLen;
                    tc.tensorName = span.name;
                    tc.dtype = span.dtype;
                    tc.isMetadata = false;

                    const uint8_t* cPtr = data + chunkStart;
                    tc.xxhash128 = Hashing::XXHashEngine::hash128(cPtr, chunkLen);
                    tc.sha256 = calcSHA256(std::string(reinterpret_cast<const char*>(cPtr), chunkLen));

                    chunks.push_back(tc);
                    chunkStart = cut;
                }
            }

            return chunks;
        }
    }

    // Generic FastCDC fallback across whole file
    auto cuts = findFastCDCCutPoints(data, 0, size, profile);
    size_t chunkStart = 0;

    for (size_t cut : cuts) {
        size_t chunkLen = cut - chunkStart;
        TensorChunk tc;
        tc.chunkIndex = chunks.size();
        tc.offset = chunkStart;
        tc.length = chunkLen;
        tc.tensorName = "generic_payload";
        tc.dtype = "RAW";
        tc.isMetadata = false;

        const uint8_t* cPtr = data + chunkStart;
        tc.xxhash128 = Hashing::XXHashEngine::hash128(cPtr, chunkLen);
        tc.sha256 = calcSHA256(std::string(reinterpret_cast<const char*>(cPtr), chunkLen));

        chunks.push_back(tc);
        chunkStart = cut;
    }

    return chunks;
}

} // namespace Core::Chunking
