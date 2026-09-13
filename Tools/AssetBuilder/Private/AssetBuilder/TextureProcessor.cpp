#include <AssetBuilder/ArtifactWriter.h>
#include <AssetBuilder/TextureProcessor.h>

#include <Graphics/Assets/Assets.h>

namespace FE::AssetBuilder
{
    using namespace Graphics;

    namespace
    {
        constexpr uint32_t kDdsMagic = Math::MakeFourCC('D', 'D', 'S', ' ');
        constexpr uint32_t kFourCcDx10 = Math::MakeFourCC('D', 'X', '1', '0');
        constexpr uint32_t kDdsPixelFormatFourCc = 0x4;
        constexpr uint32_t kDdsPixelFormatRgb = 0x40;
        constexpr uint32_t kDdsCaps2Cubemap = 0x200;
        constexpr uint32_t kDdsResourceMiscTextureCube = 0x4;

        struct DdsPixelFormat final
        {
            uint32_t m_size;
            uint32_t m_flags;
            uint32_t m_fourCc;
            uint32_t m_rgbBitCount;
            uint32_t m_rBitMask;
            uint32_t m_gBitMask;
            uint32_t m_bBitMask;
            uint32_t m_aBitMask;
        };


        struct DdsHeader final
        {
            uint32_t m_size;
            uint32_t m_flags;
            uint32_t m_height;
            uint32_t m_width;
            uint32_t m_pitchOrLinearSize;
            uint32_t m_depth;
            uint32_t m_mipMapCount;
            uint32_t m_reserved[11];
            DdsPixelFormat m_pixelFormat;
            uint32_t m_caps;
            uint32_t m_caps2;
            uint32_t m_caps3;
            uint32_t m_caps4;
            uint32_t m_reserved2;
        };


        struct DdsHeaderDx10 final
        {
            uint32_t m_dxgiFormat;
            uint32_t m_resourceDimension;
            uint32_t m_miscFlag;
            uint32_t m_arraySize;
            uint32_t m_miscFlags2;
        };

        static_assert(sizeof(DdsPixelFormat) == 32);
        static_assert(sizeof(DdsHeader) == 124);
        static_assert(sizeof(DdsHeaderDx10) == 20);


        Core::Format TranslateDxgiFormat(const uint32_t format)
        {
            switch (format)
            {
            case 2:
                return Core::Format::kR32G32B32A32_SFLOAT;
            case 3:
                return Core::Format::kR32G32B32A32_UINT;
            case 4:
                return Core::Format::kR32G32B32A32_SINT;
            case 10:
                return Core::Format::kR16G16B16A16_SFLOAT;
            case 11:
                return Core::Format::kR16G16B16A16_UNORM;
            case 12:
                return Core::Format::kR16G16B16A16_UINT;
            case 13:
                return Core::Format::kR16G16B16A16_SNORM;
            case 14:
                return Core::Format::kR16G16B16A16_SINT;
            case 16:
                return Core::Format::kR32G32_SFLOAT;
            case 17:
                return Core::Format::kR32G32_UINT;
            case 18:
                return Core::Format::kR32G32_SINT;
            case 24:
                return Core::Format::kA2R10G10B10_UNORM;
            case 25:
                return Core::Format::kA2R10G10B10_UINT;
            case 26:
                return Core::Format::kB10G11R11_UFLOAT;
            case 28:
                return Core::Format::kR8G8B8A8_UNORM;
            case 29:
                return Core::Format::kR8G8B8A8_SRGB;
            case 30:
                return Core::Format::kR8G8B8A8_UINT;
            case 31:
                return Core::Format::kR8G8B8A8_SNORM;
            case 32:
                return Core::Format::kR8G8B8A8_SINT;
            case 34:
                return Core::Format::kR16G16_SFLOAT;
            case 35:
                return Core::Format::kR16G16_UNORM;
            case 36:
                return Core::Format::kR16G16_UINT;
            case 37:
                return Core::Format::kR16G16_SNORM;
            case 38:
                return Core::Format::kR16G16_SINT;
            case 41:
                return Core::Format::kR32_SFLOAT;
            case 42:
                return Core::Format::kR32_UINT;
            case 43:
                return Core::Format::kR32_SINT;
            case 49:
                return Core::Format::kR8G8_UNORM;
            case 50:
                return Core::Format::kR8G8_UINT;
            case 51:
                return Core::Format::kR8G8_SNORM;
            case 52:
                return Core::Format::kR8G8_SINT;
            case 54:
                return Core::Format::kR16_SFLOAT;
            case 56:
                return Core::Format::kR16_UNORM;
            case 57:
                return Core::Format::kR16_UINT;
            case 58:
                return Core::Format::kR16_SNORM;
            case 59:
                return Core::Format::kR16_SINT;
            case 61:
                return Core::Format::kR8_UNORM;
            case 62:
                return Core::Format::kR8_UINT;
            case 63:
                return Core::Format::kR8_SNORM;
            case 64:
                return Core::Format::kR8_SINT;
            case 71:
                return Core::Format::kBC1_UNORM;
            case 72:
                return Core::Format::kBC1_SRGB;
            case 74:
                return Core::Format::kBC2_UNORM;
            case 75:
                return Core::Format::kBC2_SRGB;
            case 77:
                return Core::Format::kBC3_UNORM;
            case 78:
                return Core::Format::kBC3_SRGB;
            case 80:
                return Core::Format::kBC4_UNORM;
            case 81:
                return Core::Format::kBC4_SNORM;
            case 83:
                return Core::Format::kBC5_UNORM;
            case 84:
                return Core::Format::kBC5_SNORM;
            case 87:
                return Core::Format::kB8G8R8A8_UNORM;
            case 91:
                return Core::Format::kB8G8R8A8_SRGB;
            case 95:
                return Core::Format::kBC6H_UFLOAT;
            case 96:
                return Core::Format::kBC6H_SFLOAT;
            case 98:
                return Core::Format::kBC7_UNORM;
            case 99:
                return Core::Format::kBC7_SRGB;
            default:
                return Core::Format::kUndefined;
            }
        }


        Core::Format TranslateLegacyFormat(const DdsPixelFormat& format)
        {
            if ((format.m_flags & kDdsPixelFormatFourCc) != 0)
            {
                switch (format.m_fourCc)
                {
                case Math::MakeFourCC('D', 'X', 'T', '1'):
                    return Core::Format::kBC1_UNORM;
                case Math::MakeFourCC('D', 'X', 'T', '3'):
                    return Core::Format::kBC2_UNORM;
                case Math::MakeFourCC('D', 'X', 'T', '5'):
                    return Core::Format::kBC3_UNORM;
                case Math::MakeFourCC('A', 'T', 'I', '1'):
                case Math::MakeFourCC('B', 'C', '4', 'U'):
                    return Core::Format::kBC4_UNORM;
                case Math::MakeFourCC('B', 'C', '4', 'S'):
                    return Core::Format::kBC4_SNORM;
                case Math::MakeFourCC('A', 'T', 'I', '2'):
                case Math::MakeFourCC('B', 'C', '5', 'U'):
                    return Core::Format::kBC5_UNORM;
                case Math::MakeFourCC('B', 'C', '5', 'S'):
                    return Core::Format::kBC5_SNORM;
                default:
                    return Core::Format::kUndefined;
                }
            }

            if ((format.m_flags & kDdsPixelFormatRgb) == 0 || format.m_rgbBitCount != 32)
                return Core::Format::kUndefined;

            const bool isRgba = format.m_rBitMask == 0x000000ff && format.m_gBitMask == 0x0000ff00
                && format.m_bBitMask == 0x00ff0000 && format.m_aBitMask == 0xff000000;
            if (isRgba)
                return Core::Format::kR8G8B8A8_UNORM;

            const bool isBgra = format.m_rBitMask == 0x00ff0000 && format.m_gBitMask == 0x0000ff00
                && format.m_bBitMask == 0x000000ff && format.m_aBitMask == 0xff000000;
            return isBgra ? Core::Format::kB8G8R8A8_UNORM : Core::Format::kUndefined;
        }


        struct ParsedDds final
        {
            Core::TextureDesc m_desc;
            festd::inline_vector<festd::vector<std::byte>, 8> m_mips;
        };


        bool ParseDds(const festd::span<const std::byte> bytes, ParsedDds& result)
        {
            if (bytes.size() < sizeof(uint32_t) + sizeof(DdsHeader))
                return false;

            Memory::BlockReader reader(bytes);
            uint32_t magic;
            DdsHeader header;
            if (!reader.ReadBytes(&magic, sizeof(magic)) || !reader.ReadBytes(&header, sizeof(header)) || magic != kDdsMagic
                || header.m_size != sizeof(DdsHeader) || header.m_pixelFormat.m_size != sizeof(DdsPixelFormat))
            {
                return false;
            }

            DdsHeaderDx10 dx10{};
            const bool hasDx10Header = header.m_pixelFormat.m_fourCc == kFourCcDx10;
            if (hasDx10Header && !reader.ReadBytes(&dx10, sizeof(dx10)))
                return false;

            const Core::Format format =
                hasDx10Header ? TranslateDxgiFormat(dx10.m_dxgiFormat) : TranslateLegacyFormat(header.m_pixelFormat);
            if (format == Core::Format::kUndefined)
                return false;

            const uint32_t mipCount = Math::Max(header.m_mipMapCount, 1u);
            uint32_t arraySize = hasDx10Header ? dx10.m_arraySize : 1;
            Core::TextureDimension dimension = Core::TextureDimension::k2D;
            if (hasDx10Header)
            {
                if (dx10.m_resourceDimension == 2)
                    dimension = Core::TextureDimension::k1D;
                else if (dx10.m_resourceDimension == 4)
                    dimension = Core::TextureDimension::k3D;
                else if (dx10.m_resourceDimension != 3)
                    return false;

                if ((dx10.m_miscFlag & kDdsResourceMiscTextureCube) != 0)
                {
                    dimension = Core::TextureDimension::kCubemap;
                    arraySize *= 6;
                }
            }
            else if ((header.m_caps2 & kDdsCaps2Cubemap) != 0)
            {
                dimension = Core::TextureDimension::kCubemap;
                arraySize = 6;
            }

            const uint32_t depth = dimension == Core::TextureDimension::k3D ? Math::Max(header.m_depth, 1u) : 1u;
            if (dimension == Core::TextureDimension::k3D && arraySize != 1)
                return false;

            const bool dimensionsValid = header.m_width > 0 && header.m_height > 0 && header.m_width < (1u << 14)
                && header.m_height < (1u << 14) && depth < (1u << 14);
            const bool subresourcesValid = arraySize > 0 && arraySize < (1u << 12) && mipCount < (1u << 4);
            if (!dimensionsValid || !subresourcesValid)
                return false;

            result.m_desc.m_width = header.m_width;
            result.m_desc.m_height = dimension == Core::TextureDimension::k1D ? 1 : header.m_height;
            result.m_desc.m_depth = depth;
            result.m_desc.m_arraySize = arraySize;
            result.m_desc.m_mipSliceCount = mipCount;
            result.m_desc.m_sampleCount = 1;
            result.m_desc.m_dimension = dimension;
            result.m_desc.m_imageFormat = format;

            result.m_mips.resize(mipCount);
            const Core::FormatInfo formatInfo(format);
            for (uint32_t arrayIndex = 0; arrayIndex < arraySize; ++arrayIndex)
            {
                for (uint32_t sourceMip = 0; sourceMip < mipCount; ++sourceMip)
                {
                    const uint32_t mipSize =
                        formatInfo.CalculateMipByteSize({ header.m_width, result.m_desc.m_height, depth }, sourceMip);
                    if (reader.m_ptr + mipSize > reader.m_end)
                        return false;

                    const uint32_t destinationMip = mipCount - sourceMip - 1;
                    festd::vector<std::byte>& mip = result.m_mips[destinationMip];
                    const uint32_t oldSize = mip.size();
                    mip.resize(oldSize + mipSize);
                    memcpy(mip.data() + oldSize, reader.m_ptr, mipSize);
                    reader.m_ptr += mipSize;
                }

                if (dimension == Core::TextureDimension::k3D)
                    break;
            }
            return true;
        }
    } // namespace


    bool ValidateTextureSource(const IO::Path& path, const festd::span<const std::byte> sourceData)
    {
        ParsedDds dds;
        if (ParseDds(sourceData, dds))
            return true;

        Logger::LogError("Unsupported or malformed DDS texture '{}'", path);
        return false;
    }


    bool ProcessTexture(const TextureProcessSettings& settings)
    {
        ParsedDds dds;
        if (!ParseDds(settings.m_sourceData, dds))
        {
            Logger::LogError("Unsupported or malformed DDS texture '{}'", settings.m_inputFile);
            return false;
        }

        TextureAsset header;
        header.m_desc = dds.m_desc;
        uint32_t tailMipCount = 0;
        for (const festd::vector<std::byte>& mip : dds.m_mips)
        {
            if (header.m_mipTailData.size() + mip.size() > TextureAsset::kMaxMipTailByteSize)
                break;

            header.m_mipTailOffsets.push_back(header.m_mipTailData.size());
            header.m_mipTailData.insert(header.m_mipTailData.end(), mip.begin(), mip.end());
            ++tailMipCount;
        }

        ArtifactWriter writer(settings.m_outputDirectory,
                              settings.m_assetId,
                              settings.m_artifactId,
                              Rtti::GetTypeID<TextureAsset>());
        if (!writer.WriteHeader(header))
            return false;

        for (uint32_t mipIndex = tailMipCount; mipIndex < dds.m_mips.size(); ++mipIndex)
        {
            if (!writer.WritePayload(dds.m_mips[mipIndex]))
                return false;
        }

        return writer.Finish();
    }
} // namespace FE::AssetBuilder
