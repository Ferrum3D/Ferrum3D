#pragma once
#include <FeCore/Strings/StringSlice.h>
#include <FeCore/Utils/Result.h>
#include <HAL/ImageEnums.h>
#include <HAL/ImageFormat.h>

namespace FE::Graphics
{
    struct RawImage final
    {
        uint8_t* Data = nullptr;
        uint16_t Width = 0;
        uint16_t Height = 0;
        uint32_t ChannelCount = 0;

        static Result<RawImage, StringSlice> LoadFromMemory(festd::span<const uint8_t> data);
        static void Free(RawImage& image);
    };


    namespace DDS
    {
        enum class Format : uint32_t
        {
            UNKNOWN = 0,
            R32G32B32A32_TYPELESS = 1,
            R32G32B32A32_FLOAT = 2,
            R32G32B32A32_UINT = 3,
            R32G32B32A32_SINT = 4,
            R32G32B32_TYPELESS = 5,
            R32G32B32_FLOAT = 6,
            R32G32B32_UINT = 7,
            R32G32B32_SINT = 8,
            R16G16B16A16_TYPELESS = 9,
            R16G16B16A16_FLOAT = 10,
            R16G16B16A16_UNORM = 11,
            R16G16B16A16_UINT = 12,
            R16G16B16A16_SNORM = 13,
            R16G16B16A16_SINT = 14,
            R32G32_TYPELESS = 15,
            R32G32_FLOAT = 16,
            R32G32_UINT = 17,
            R32G32_SINT = 18,
            R32G8X24_TYPELESS = 19,
            D32_FLOAT_S8X24_UINT = 20,
            R32_FLOAT_X8X24_TYPELESS = 21,
            X32_TYPELESS_G8X24_UINT = 22,
            R10G10B10A2_TYPELESS = 23,
            R10G10B10A2_UNORM = 24,
            R10G10B10A2_UINT = 25,
            R11G11B10_FLOAT = 26,
            R8G8B8A8_TYPELESS = 27,
            R8G8B8A8_UNORM = 28,
            R8G8B8A8_UNORM_SRGB = 29,
            R8G8B8A8_UINT = 30,
            R8G8B8A8_SNORM = 31,
            R8G8B8A8_SINT = 32,
            R16G16_TYPELESS = 33,
            R16G16_FLOAT = 34,
            R16G16_UNORM = 35,
            R16G16_UINT = 36,
            R16G16_SNORM = 37,
            R16G16_SINT = 38,
            R32_TYPELESS = 39,
            D32_FLOAT = 40,
            R32_FLOAT = 41,
            R32_UINT = 42,
            R32_SINT = 43,
            R24G8_TYPELESS = 44,
            D24_UNORM_S8_UINT = 45,
            R24_UNORM_X8_TYPELESS = 46,
            X24_TYPELESS_G8_UINT = 47,
            R8G8_TYPELESS = 48,
            R8G8_UNORM = 49,
            R8G8_UINT = 50,
            R8G8_SNORM = 51,
            R8G8_SINT = 52,
            R16_TYPELESS = 53,
            R16_FLOAT = 54,
            D16_UNORM = 55,
            R16_UNORM = 56,
            R16_UINT = 57,
            R16_SNORM = 58,
            R16_SINT = 59,
            R8_TYPELESS = 60,
            R8_UNORM = 61,
            R8_UINT = 62,
            R8_SNORM = 63,
            R8_SINT = 64,
            A8_UNORM = 65,
            R1_UNORM = 66,
            R9G9B9E5_SHAREDEXP = 67,
            R8G8_B8G8_UNORM = 68,
            G8R8_G8B8_UNORM = 69,
            BC1_TYPELESS = 70,
            BC1_UNORM = 71,
            BC1_SRGB = 72,
            BC2_TYPELESS = 73,
            BC2_UNORM = 74,
            BC2_SRGB = 75,
            BC3_TYPELESS = 76,
            BC3_UNORM = 77,
            BC3_SRGB = 78,
            BC4_TYPELESS = 79,
            BC4_UNORM = 80,
            BC4_SNORM = 81,
            BC5_TYPELESS = 82,
            BC5_UNORM = 83,
            BC5_SNORM = 84,
            B5G6R5_UNORM = 85,
            B5G5R5A1_UNORM = 86,
            B8G8R8A8_UNORM = 87,
            B8G8R8X8_UNORM = 88,
            R10G10B10_XR_BIAS_A2_UNORM = 89,
            B8G8R8A8_TYPELESS = 90,
            B8G8R8A8_SRGB = 91,
            B8G8R8X8_TYPELESS = 92,
            B8G8R8X8_SRGB = 93,
            BC6H_TYPELESS = 94,
            BC6H_UFLOAT = 95,
            BC6H_SFLOAT = 96,
            BC7_TYPELESS = 97,
            BC7_UNORM = 98,
            BC7_SRGB = 99
        };


        enum class ResourceDimension : uint32_t
        {
            kUnknown = 0,
            kBuffer = 1,
            kTexture1D = 2,
            kTexture2D = 3,
            kTexture3D = 4
        };


        enum class DDSDFlags
        {
            kCaps = 0x00000001,
            kHeight = 0x00000002,
            kWidth = 0x00000004,
            kPitch = 0x00000008,
            kPixelFormat = 0x00001000,
            kMipmapCount = 0x00020000,
            kLinearSize = 0x00080000,
            kDepth = 0x00800000,
        };

        FE_ENUM_OPERATORS(DDSDFlags);


        struct PixelFormat final
        {
            uint32_t dwSize;
            uint32_t dwFlags;
            uint32_t dwFourCC;
            uint32_t dwRGBBitCount;
            uint32_t dwRBitMask;
            uint32_t dwGBitMask;
            uint32_t dwBBitMask;
            uint32_t dwABitMask;
        };


        struct BaseHeader final
        {
            uint32_t dwSize;
            DDSDFlags dwFlags;
            uint32_t dwHeight;
            uint32_t dwWidth;
            uint32_t dwPitchOrLinearSize;
            uint32_t dwDepth;
            uint32_t dwMipMapCount;
            uint32_t dwReserved1[11];
            PixelFormat ddspf;
            uint32_t dwCaps;
            uint32_t dwCaps2;
            uint32_t dwCaps3;
            uint32_t dwCaps4;
            uint32_t dwReserved2;
        };


        struct HeaderDXT10 final
        {
            Format dxgiFormat;
            ResourceDimension resourceDimension;
            uint32_t miscFlag;
            uint32_t arraySize;
            uint32_t miscFlags2;
        };


        struct Header final
        {
            BaseHeader header;
            HeaderDXT10 headerDX10;
        };


        inline static bool CheckHeader(const Header* header)
        {
            const BaseHeader* baseHeader = &header->header;
            if (baseHeader->dwSize != sizeof(BaseHeader))
                return false;

            const DDSDFlags kRequiredFlags = DDSDFlags::kHeight | DDSDFlags::kWidth;
            if ((baseHeader->dwFlags & kRequiredFlags) != kRequiredFlags)
                return false;

            if (baseHeader->ddspf.dwSize != sizeof(PixelFormat))
                return false;

            if (baseHeader->ddspf.dwFourCC != Math::MakeFourCC('D', 'X', '1', '0'))
                return false;

            return true;
        }


        inline static HAL::ImageDim ConvertDimension(ResourceDimension dimension)
        {
            switch (dimension)
            {
            case ResourceDimension::kTexture1D:
                return HAL::ImageDim::kImage1D;
            case ResourceDimension::kTexture2D:
                return HAL::ImageDim::kImage2D;
            case ResourceDimension::kTexture3D:
                return HAL::ImageDim::kImage3D;
            default:
                return HAL::ImageDim::kImage2D;
            }
        }


        inline static HAL::Format ConvertFormat(Format dxgiFormat)
        {
            switch (dxgiFormat)
            {
#define FE_EXPAND_DXGI_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                 \
    case Format::name:                                                                                                           \
        return HAL::Format::k##name;

                FE_EXPAND_BC_FORMATS(FE_EXPAND_DXGI_FORMAT_CONVERSION)

#undef FE_EXPAND_DXGI_FORMAT_CONVERSION

            case Format::R32G32B32A32_FLOAT:
                return HAL::Format::kR32G32B32A32_SFLOAT;
            case Format::R8G8B8A8_UNORM:
                return HAL::Format::kR8G8B8A8_UNORM;

            default:
                return HAL::Format::kUndefined;
            }
        }
    } // namespace DDS
} // namespace FE::Graphics
