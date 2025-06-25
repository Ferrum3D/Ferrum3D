#include "TextureProcessor.h"
#include "Utils.h"

#include <FeCore/Compression/Compression.h>
#include <FeCore/IO/IStreamFactory.h>
#include <FeCore/Math/Color.h>
#include <FeCore/Memory/SegmentedBuffer.h>
#include <Graphics/Assets/TextureAssetFormat.h>
#include <Graphics/Core/Base/Limits.h>
#include <Graphics/Core/ImageBase.h>
#include <festd/vector.h>

#include <cmp_core.h>

#define STBI_MALLOC(size) FE::Memory::DefaultAllocate(size)
#define STBI_REALLOC(p, newSize) FE::Memory::DefaultReallocate(p, newSize)
#define STBI_FREE(p) FE::Memory::DefaultFree(p)

#define STBIR_MALLOC(size, c) ((void)(c), FE::Memory::DefaultAllocate(size))
#define STBIR_FREE(p, c) ((void)(c), FE::Memory::DefaultFree(p))

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

using namespace FE::Graphics;

namespace FE
{
    namespace
    {
        std::byte* CompressTextureBC7(const float* sourceData, const uint32_t width, const uint32_t height)
        {
            const uint32_t blockCountX = Math::CeilDivide(width, 4);
            const uint32_t blockCountY = Math::CeilDivide(height, 4);

            void* options;
            CreateOptionsBC7(&options);
            SetMaskBC7(options, 1 << 6);
            SetQualityBC7(options, 0.3f);

            auto* compressedData = static_cast<std::byte*>(Memory::DefaultAllocate(blockCountX * blockCountY * 16));
            for (uint32_t yb = 0; yb < blockCountY; ++yb)
            {
                for (uint32_t xb = 0; xb < blockCountX; ++xb)
                {
                    struct BlockPixel
                    {
                        uint8_t r, g, b, a;
                    };

                    BlockPixel blockData[16];
                    for (uint32_t y = 0; y < 4; ++y)
                    {
                        for (uint32_t x = 0; x < 4; ++x)
                        {
                            const uint32_t xPixel = xb * 4 + x;
                            const uint32_t yPixel = yb * 4 + y;
                            if (xPixel >= width || yPixel >= height)
                            {
                                Memory::Zero(&blockData[y * 4 + x], sizeof(BlockPixel));
                                blockData[y * 4 + x].a = 0x0;
                                continue;
                            }

                            const __m128 pixelColor = _mm_load_ps(sourceData + (yPixel * width + xPixel) * 4);
                            const __m128i t = _mm_cvttps_epi32(_mm_mul_ps(pixelColor, _mm_set1_ps(255.0f)));
                            const __m128i v = _mm_min_epi32(t, _mm_set1_epi32(255));
                            const uint32_t r = static_cast<uint32_t>(_mm_extract_epi8(v, 4 * 0));
                            const uint32_t g = static_cast<uint32_t>(_mm_extract_epi8(v, 4 * 1));
                            const uint32_t b = static_cast<uint32_t>(_mm_extract_epi8(v, 4 * 2));
                            // const uint32_t a = static_cast<uint32_t>(_mm_extract_epi8(v, 4 * 3));

                            blockData[y * 4 + x].r = r & 0xff;
                            blockData[y * 4 + x].g = g & 0xff;
                            blockData[y * 4 + x].b = b & 0xff;
                            blockData[y * 4 + x].a = 0x0;
                        }
                    }

                    uint8_t compressedBlock[16];
                    CompressBlockBC7(reinterpret_cast<uint8_t*>(blockData), 16, compressedBlock, options);

                    const __m128 block = _mm_loadu_ps(reinterpret_cast<const float*>(compressedBlock));
                    _mm_store_ps(reinterpret_cast<float*>(compressedData + (yb * blockCountX + xb) * 16), block);
                }
            }

            return compressedData;
        }
    } // namespace


    bool AssetBuilder::ProcessTexture(const TextureProcessSettings& settings)
    {
        auto fileResult = settings.m_streamFactory->OpenFileStream(settings.m_inputFile, IO::OpenMode::kReadOnly);
        if (!fileResult)
        {
            settings.m_logger->LogError(
                "Failed to open file {}: {}", settings.m_inputFile, IO::GetResultDesc(fileResult.error()));
            return false;
        }

        settings.m_logger->LogInfo("Processing texture {}", settings.m_inputFile);

        IO::IStream* file = fileResult->Get();

        const size_t rawSize = file->Length();
        void* rawData = Memory::DefaultAllocate(rawSize);
        if (file->ReadToBuffer(rawData, rawSize) != rawSize)
        {
            Memory::DefaultFree(rawData);
            settings.m_logger->LogError("Failed to read file {}", settings.m_inputFile);
            return false;
        }

        fileResult->Reset();
        file = nullptr;

        int32_t sourceWidth, sourceHeight, sourceChannels;
        float* imageData = stbi_loadf_from_memory(
            static_cast<stbi_uc*>(rawData), static_cast<int32_t>(rawSize), &sourceWidth, &sourceHeight, &sourceChannels, 4);

        Memory::DefaultFree(rawData);

        if (imageData == nullptr)
        {
            settings.m_logger->LogError("Failed to load image {}: {}", settings.m_inputFile, stbi_failure_reason());
            return false;
        }

        Vector2UInt outputSize = settings.m_outputSize;
        if (outputSize == Vector2UInt::Zero())
        {
            outputSize = Vector2UInt(sourceWidth, sourceHeight);
        }

        if (outputSize != Vector2UInt(sourceWidth, sourceHeight))
        {
            settings.m_logger->LogInfo(
                "Resizing image from {}x{} to {}x{}", sourceWidth, sourceHeight, outputSize.x, outputSize.y);

            const size_t outputWidth = outputSize.x;
            const size_t outputHeight = outputSize.y;
            auto* resizedData = static_cast<float*>(Memory::DefaultAllocate(outputWidth * outputHeight * 4 * sizeof(float)));

            stbir_resize_float_generic(imageData,
                                       sourceWidth,
                                       sourceHeight,
                                       0,
                                       resizedData,
                                       static_cast<int32_t>(outputWidth),
                                       static_cast<int32_t>(outputHeight),
                                       0,
                                       4,
                                       3,
                                       0,
                                       STBIR_EDGE_CLAMP,
                                       STBIR_FILTER_MITCHELL,
                                       STBIR_COLORSPACE_LINEAR,
                                       nullptr);

            stbi_image_free(imageData);
            imageData = resizedData;
        }

        const Core::FormatInfo formatInfo{ settings.m_format };
        if (formatInfo.GetChannelCount() < static_cast<uint32_t>(sourceChannels))
        {
            settings.m_logger->LogError("Image {} has {} channels, but requested format {} has {} channels",
                                        settings.m_inputFile,
                                        sourceChannels,
                                        settings.m_format,
                                        formatInfo.GetChannelCount());
            return false;
        }

        festd::fixed_vector<float*, Core::Limits::Image::kMaxMipCount> mipData;
        mipData.push_back(imageData);

        const uint32_t mipCount = settings.m_generateMips ? Core::CalculateMipCount(outputSize) : 1;
        if (mipCount > 1)
        {
            settings.m_logger->LogInfo("Generating {} mipmaps", mipCount - 1);
        }

        for (uint32_t mipIndex = 1; mipIndex < mipCount; ++mipIndex)
        {
            const uint32_t sourceMipIndex = mipIndex - 1;
            const uint32_t sourceMipWidth = outputSize.x >> sourceMipIndex;
            const uint32_t sourceMipHeight = outputSize.y >> sourceMipIndex;
            const float* sourceMipData = mipData[sourceMipIndex];

            const size_t mipWidth = Math::Max(1u, outputSize.x >> mipIndex);
            const size_t mipHeight = Math::Max(1u, outputSize.y >> mipIndex);
            auto* mipDataPtr = static_cast<float*>(Memory::DefaultAllocate(mipWidth * mipHeight * 4 * sizeof(float)));

            stbir_resize_float_generic(sourceMipData,
                                       static_cast<int32_t>(sourceMipWidth),
                                       static_cast<int32_t>(sourceMipHeight),
                                       0,
                                       mipDataPtr,
                                       static_cast<int32_t>(mipWidth),
                                       static_cast<int32_t>(mipHeight),
                                       0,
                                       4,
                                       3,
                                       0,
                                       STBIR_EDGE_CLAMP,
                                       STBIR_FILTER_BOX,
                                       STBIR_COLORSPACE_LINEAR,
                                       nullptr);
            mipData.push_back(mipDataPtr);

            settings.m_logger->LogInfo("Generated mips [{}/{}]", mipIndex + 1, mipCount);
        }

        settings.m_logger->LogInfo("Compressing texture to format {}", settings.m_format);

        festd::fixed_vector<std::byte*, Core::Limits::Image::kMaxMipCount> blockCompressedMipData;
        auto deferDeleteData = festd::defer([&blockCompressedMipData] {
            for (std::byte* data : blockCompressedMipData)
                Memory::DefaultFree(data);
        });

        for (uint32_t mipIndex = 0; mipIndex < mipCount; ++mipIndex)
        {
            const uint32_t width = Math::Max(1u, outputSize.x >> mipIndex);
            const uint32_t height = Math::Max(1u, outputSize.y >> mipIndex);

            if (formatInfo.m_isBlockCompressed)
            {
                switch (settings.m_format)
                {
                default:
                    // not implemented
                    FE_DebugBreak();
                    break;

                case Core::Format::kBC7_UNORM:
                    blockCompressedMipData.push_back(CompressTextureBC7(mipData[mipIndex], width, height));
                    break;
                }
            }

            settings.m_logger->LogInfo("Compressed mips [{}/{}]", mipIndex + 1, mipCount);
        }

        for (float* data : mipData)
            Memory::DefaultFree(data);
        mipData.clear();

        const auto compressor = Compression::Compressor::Create(Compression::Method::kGDeflate);

        //
        // The first block of the texture file contains the header, followed by an array of MipChainInfo structs.
        // We also try to store there as many small mips as possible to read them all in one go together with the header.
        //

        Data::TextureHeader header;
        header.m_magic = Data::kTextureMagic;
        header.m_desc.m_width = outputSize.x;
        header.m_desc.m_height = outputSize.y;
        header.m_desc.m_sampleCount = 1;
        header.m_desc.m_depth = 1;
        header.m_desc.m_arraySize = 1;
        header.m_desc.m_mipSliceCount = mipCount;
        header.m_desc.m_dimension = Core::ImageDimension::k2D;
        header.m_desc.m_imageFormat = settings.m_format;

        festd::fixed_vector<Data::MipChainInfo, Core::Limits::Image::kMaxMipCount> mipChainInfo;

        bool hasMipsInFirstBlock = false;
        uint32_t firstBlockBytes = sizeof(Data::TextureHeader);

        for (uint32_t i = mipCount; i > 0; --i)
        {
            const uint32_t mipIndex = i - 1;
            const uint32_t mipBytes = formatInfo.CalculateMipByteSize(outputSize, mipIndex);

            const uint32_t expectedFirstBlockSize = firstBlockBytes + mipBytes + sizeof(Data::MipChainInfo) * i;
            if (expectedFirstBlockSize <= Compression::kBlockSize)
            {
                firstBlockBytes += mipBytes;

                if (mipChainInfo.empty())
                {
                    FE_Assert(!hasMipsInFirstBlock);
                    firstBlockBytes += sizeof(Data::MipChainInfo);
                    mipChainInfo.push_back({});
                    hasMipsInFirstBlock = true;
                }

                FE_Assert(mipChainInfo.size() == 1);

                Data::MipChainInfo& firstMipInfo = mipChainInfo[0];
                firstMipInfo.m_reserved = 0;
                firstMipInfo.m_arraySlice = 0;
                firstMipInfo.m_blockCount = 1;
                firstMipInfo.m_mostDetailedMipSlice = mipIndex;
                ++firstMipInfo.m_mipSliceCount;
            }
            else
            {
                Data::MipChainInfo& mipInfo = mipChainInfo.push_back();
                mipInfo.m_reserved = 0;
                mipInfo.m_arraySlice = 0;
                mipInfo.m_blockCount = Math::CeilDivide(mipBytes, Compression::kBlockSize);
                mipInfo.m_mostDetailedMipSlice = mipIndex;
                mipInfo.m_mipSliceCount = 1;

                firstBlockBytes += sizeof(Data::MipChainInfo);
            }
        }

        auto outFileResult = settings.m_streamFactory->OpenFileStream(settings.m_outputFile, IO::OpenMode::kCreate);
        if (!outFileResult)
        {
            settings.m_logger->LogError(
                "Failed to open file {}: {}", settings.m_outputFile, IO::GetResultDesc(outFileResult.error()));
            return false;
        }

        IO::IStream* out = outFileResult->Get();

        Crc32 crc32;

        festd::vector<std::byte> tempCompressedBuffer{ static_cast<uint32_t>(compressor.GetBounds(Compression::kBlockSize)) };

        {
            settings.m_logger->LogInfo("Compressing final data using GDeflate");

            festd::vector<std::byte> tempUncompressedBuffer{ Compression::kBlockSize };
            Memory::BlockWriter writer{ tempUncompressedBuffer };
            writer.Write(header);
            FE_Verify(writer.WriteBytes(mipChainInfo.data(), festd::size_bytes(mipChainInfo)));

            if (hasMipsInFirstBlock)
            {
                const Data::MipChainInfo& firstMipInfo = mipChainInfo[0];
                for (uint32_t mipIndex = firstMipInfo.m_mostDetailedMipSlice;
                     mipIndex < firstMipInfo.m_mostDetailedMipSlice + firstMipInfo.m_mipSliceCount;
                     ++mipIndex)
                {
                    const uint32_t mipBytes = formatInfo.CalculateMipByteSize(outputSize, mipIndex);
                    FE_Verify(writer.WriteBytes(blockCompressedMipData[mipIndex], mipBytes));
                }

                FE_Assert(writer.m_ptr - tempUncompressedBuffer.data() == firstBlockBytes);
                FE_Assert(firstBlockBytes <= Compression::kBlockSize);
            }

            FE_Verify(compressor.Compress(crc32,
                                          tempUncompressedBuffer.data(),
                                          writer.m_ptr - tempUncompressedBuffer.data(),
                                          tempCompressedBuffer.data(),
                                          tempCompressedBuffer.size()));

            WriteCompactedPages(tempCompressedBuffer, out);

            if (hasMipsInFirstBlock)
                settings.m_logger->LogInfo("Compressed mip chains [1/{}]", mipChainInfo.size());
        }

        CompressedBlockWriter compressedBlockWriter{ out, &compressor, crc32 };
        for (uint32_t mipChainIndex = 0; mipChainIndex < mipChainInfo.size(); ++mipChainIndex)
        {
            if (mipChainIndex == 0 && hasMipsInFirstBlock)
                continue;

            const Data::MipChainInfo& mipInfo = mipChainInfo[mipChainIndex];
            FE_Assert(mipInfo.m_mipSliceCount == 1);

            compressedBlockWriter.WriteBytes(blockCompressedMipData[mipInfo.m_mostDetailedMipSlice],
                                             formatInfo.CalculateMipByteSize(outputSize, mipInfo.m_mostDetailedMipSlice));
            compressedBlockWriter.Flush();

            settings.m_logger->LogInfo("Compressed mip chains [{}/{}]", mipChainIndex + 1, mipChainInfo.size());
        }

        compressedBlockWriter.Flush();
        outFileResult->Reset();
        out = nullptr;

        settings.m_logger->LogInfo("Done writing to file {}", settings.m_outputFile);

        return true;
    }
} // namespace FE
