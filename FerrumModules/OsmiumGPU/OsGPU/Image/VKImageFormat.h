#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Image/ImageFormat.h>
#include <array>
#include <tuple>

namespace FE::Osmium
{
    // clang-format off
    static auto VKFormatConversions = []() {
        std::array<VkFormat, static_cast<USize>(Format::Count)> result{};
        result[static_cast<USize>(Format::None)] = VK_FORMAT_UNDEFINED;
        result[static_cast<USize>(Format::R4G4_UNorm_Pack8)] = VK_FORMAT_R4G4_UNORM_PACK8;
        result[static_cast<USize>(Format::R4G4B4A4_UNorm_Pack16)] = VK_FORMAT_R4G4B4A4_UNORM_PACK16;
        result[static_cast<USize>(Format::B4G4R4A4_UNorm_Pack16)] = VK_FORMAT_B4G4R4A4_UNORM_PACK16;
        result[static_cast<USize>(Format::R5G6B5_UNorm_Pack16)] = VK_FORMAT_R5G6B5_UNORM_PACK16;
        result[static_cast<USize>(Format::B5G6R5_UNorm_Pack16)] = VK_FORMAT_B5G6R5_UNORM_PACK16;
        result[static_cast<USize>(Format::R5G5B5A1_UNorm_Pack16)] = VK_FORMAT_R5G5B5A1_UNORM_PACK16;
        result[static_cast<USize>(Format::B5G5R5A1_UNorm_Pack16)] = VK_FORMAT_B5G5R5A1_UNORM_PACK16;
        result[static_cast<USize>(Format::A1R5G5B5_UNorm_Pack16)] = VK_FORMAT_A1R5G5B5_UNORM_PACK16;
        result[static_cast<USize>(Format::R8_UNorm)] = VK_FORMAT_R8_UNORM;
        result[static_cast<USize>(Format::R8_SNorm)] = VK_FORMAT_R8_SNORM;
        result[static_cast<USize>(Format::R8_UScaled)] = VK_FORMAT_R8_USCALED;
        result[static_cast<USize>(Format::R8_SScaled)] = VK_FORMAT_R8_SSCALED;
        result[static_cast<USize>(Format::R8_UInt)] = VK_FORMAT_R8_UINT;
        result[static_cast<USize>(Format::R8_SInt)] = VK_FORMAT_R8_SINT;
        result[static_cast<USize>(Format::R8_SRGB)] = VK_FORMAT_R8_SRGB;
        result[static_cast<USize>(Format::R8G8_UNorm)] = VK_FORMAT_R8G8_UNORM;
        result[static_cast<USize>(Format::R8G8_SNorm)] = VK_FORMAT_R8G8_SNORM;
        result[static_cast<USize>(Format::R8G8_UScaled)] = VK_FORMAT_R8G8_USCALED;
        result[static_cast<USize>(Format::R8G8_SScaled)] = VK_FORMAT_R8G8_SSCALED;
        result[static_cast<USize>(Format::R8G8_UInt)] = VK_FORMAT_R8G8_UINT;
        result[static_cast<USize>(Format::R8G8_SInt)] = VK_FORMAT_R8G8_SINT;
        result[static_cast<USize>(Format::R8G8_SRGB)] = VK_FORMAT_R8G8_SRGB;
        result[static_cast<USize>(Format::R8G8B8_UNorm)] = VK_FORMAT_R8G8B8_UNORM;
        result[static_cast<USize>(Format::R8G8B8_SNorm)] = VK_FORMAT_R8G8B8_SNORM;
        result[static_cast<USize>(Format::R8G8B8_UScaled)] = VK_FORMAT_R8G8B8_USCALED;
        result[static_cast<USize>(Format::R8G8B8_SScaled)] = VK_FORMAT_R8G8B8_SSCALED;
        result[static_cast<USize>(Format::R8G8B8_UInt)] = VK_FORMAT_R8G8B8_UINT;
        result[static_cast<USize>(Format::R8G8B8_SInt)] = VK_FORMAT_R8G8B8_SINT;
        result[static_cast<USize>(Format::R8G8B8_SRGB)] = VK_FORMAT_R8G8B8_SRGB;
        result[static_cast<USize>(Format::B8G8R8_UNorm)] = VK_FORMAT_B8G8R8_UNORM;
        result[static_cast<USize>(Format::B8G8R8_SNorm)] = VK_FORMAT_B8G8R8_SNORM;
        result[static_cast<USize>(Format::B8G8R8_UScaled)] = VK_FORMAT_B8G8R8_USCALED;
        result[static_cast<USize>(Format::B8G8R8_SScaled)] = VK_FORMAT_B8G8R8_SSCALED;
        result[static_cast<USize>(Format::B8G8R8_UInt)] = VK_FORMAT_B8G8R8_UINT;
        result[static_cast<USize>(Format::B8G8R8_SInt)] = VK_FORMAT_B8G8R8_SINT;
        result[static_cast<USize>(Format::B8G8R8_SRGB)] = VK_FORMAT_B8G8R8_SRGB;
        result[static_cast<USize>(Format::R8G8B8A8_UNorm)] = VK_FORMAT_R8G8B8A8_UNORM;
        result[static_cast<USize>(Format::R8G8B8A8_SNorm)] = VK_FORMAT_R8G8B8A8_SNORM;
        result[static_cast<USize>(Format::R8G8B8A8_UScaled)] = VK_FORMAT_R8G8B8A8_USCALED;
        result[static_cast<USize>(Format::R8G8B8A8_SScaled)] = VK_FORMAT_R8G8B8A8_SSCALED;
        result[static_cast<USize>(Format::R8G8B8A8_UInt)] = VK_FORMAT_R8G8B8A8_UINT;
        result[static_cast<USize>(Format::R8G8B8A8_SInt)] = VK_FORMAT_R8G8B8A8_SINT;
        result[static_cast<USize>(Format::R8G8B8A8_SRGB)] = VK_FORMAT_R8G8B8A8_SRGB;
        result[static_cast<USize>(Format::B8G8R8A8_UNorm)] = VK_FORMAT_B8G8R8A8_UNORM;
        result[static_cast<USize>(Format::B8G8R8A8_SNorm)] = VK_FORMAT_B8G8R8A8_SNORM;
        result[static_cast<USize>(Format::B8G8R8A8_UScaled)] = VK_FORMAT_B8G8R8A8_USCALED;
        result[static_cast<USize>(Format::B8G8R8A8_SScaled)] = VK_FORMAT_B8G8R8A8_SSCALED;
        result[static_cast<USize>(Format::B8G8R8A8_UInt)] = VK_FORMAT_B8G8R8A8_UINT;
        result[static_cast<USize>(Format::B8G8R8A8_SInt)] = VK_FORMAT_B8G8R8A8_SINT;
        result[static_cast<USize>(Format::B8G8R8A8_SRGB)] = VK_FORMAT_B8G8R8A8_SRGB;
        result[static_cast<USize>(Format::A8B8G8R8_UNorm_Pack32)] = VK_FORMAT_A8B8G8R8_UNORM_PACK32;
        result[static_cast<USize>(Format::A8B8G8R8_SNorm_Pack32)] = VK_FORMAT_A8B8G8R8_SNORM_PACK32;
        result[static_cast<USize>(Format::A8B8G8R8_UScaled_Pack32)] = VK_FORMAT_A8B8G8R8_USCALED_PACK32;
        result[static_cast<USize>(Format::A8B8G8R8_SScaled_Pack32)] = VK_FORMAT_A8B8G8R8_SSCALED_PACK32;
        result[static_cast<USize>(Format::A8B8G8R8_UInt_Pack32)] = VK_FORMAT_A8B8G8R8_UINT_PACK32;
        result[static_cast<USize>(Format::A8B8G8R8_SInt_Pack32)] = VK_FORMAT_A8B8G8R8_SINT_PACK32;
        result[static_cast<USize>(Format::A8B8G8R8_SRGB_Pack32)] = VK_FORMAT_A8B8G8R8_SRGB_PACK32;
        result[static_cast<USize>(Format::A2R10G10B10_UNorm_Pack32)] = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        result[static_cast<USize>(Format::A2R10G10B10_SNorm_Pack32)] = VK_FORMAT_A2R10G10B10_SNORM_PACK32;
        result[static_cast<USize>(Format::A2R10G10B10_UScaled_Pack32)] = VK_FORMAT_A2R10G10B10_USCALED_PACK32;
        result[static_cast<USize>(Format::A2R10G10B10_SScaled_Pack32)] = VK_FORMAT_A2R10G10B10_SSCALED_PACK32;
        result[static_cast<USize>(Format::A2R10G10B10_UInt_Pack32)] = VK_FORMAT_A2R10G10B10_UINT_PACK32;
        result[static_cast<USize>(Format::A2R10G10B10_SInt_Pack32)] = VK_FORMAT_A2R10G10B10_SINT_PACK32;
        result[static_cast<USize>(Format::A2B10G10R10_UNorm_Pack32)] = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        result[static_cast<USize>(Format::A2B10G10R10_SNorm_Pack32)] = VK_FORMAT_A2B10G10R10_SNORM_PACK32;
        result[static_cast<USize>(Format::A2B10G10R10_UScaled_Pack32)] = VK_FORMAT_A2B10G10R10_USCALED_PACK32;
        result[static_cast<USize>(Format::A2B10G10R10_SScaled_Pack32)] = VK_FORMAT_A2B10G10R10_SSCALED_PACK32;
        result[static_cast<USize>(Format::A2B10G10R10_UInt_Pack32)] = VK_FORMAT_A2B10G10R10_UINT_PACK32;
        result[static_cast<USize>(Format::A2B10G10R10_SInt_Pack32)] = VK_FORMAT_A2B10G10R10_SINT_PACK32;
        result[static_cast<USize>(Format::R16_UNorm)] = VK_FORMAT_R16_UNORM;
        result[static_cast<USize>(Format::R16_SNorm)] = VK_FORMAT_R16_SNORM;
        result[static_cast<USize>(Format::R16_UScaled)] = VK_FORMAT_R16_USCALED;
        result[static_cast<USize>(Format::R16_SScaled)] = VK_FORMAT_R16_SSCALED;
        result[static_cast<USize>(Format::R16_UInt)] = VK_FORMAT_R16_UINT;
        result[static_cast<USize>(Format::R16_SInt)] = VK_FORMAT_R16_SINT;
        result[static_cast<USize>(Format::R16_SFloat)] = VK_FORMAT_R16_SFLOAT;
        result[static_cast<USize>(Format::R16G16_UNorm)] = VK_FORMAT_R16G16_UNORM;
        result[static_cast<USize>(Format::R16G16_SNorm)] = VK_FORMAT_R16G16_SNORM;
        result[static_cast<USize>(Format::R16G16_UScaled)] = VK_FORMAT_R16G16_USCALED;
        result[static_cast<USize>(Format::R16G16_SScaled)] = VK_FORMAT_R16G16_SSCALED;
        result[static_cast<USize>(Format::R16G16_UInt)] = VK_FORMAT_R16G16_UINT;
        result[static_cast<USize>(Format::R16G16_SInt)] = VK_FORMAT_R16G16_SINT;
        result[static_cast<USize>(Format::R16G16_SFloat)] = VK_FORMAT_R16G16_SFLOAT;
        result[static_cast<USize>(Format::R16G16B16_UNorm)] = VK_FORMAT_R16G16B16_UNORM;
        result[static_cast<USize>(Format::R16G16B16_SNorm)] = VK_FORMAT_R16G16B16_SNORM;
        result[static_cast<USize>(Format::R16G16B16_UScaled)] = VK_FORMAT_R16G16B16_USCALED;
        result[static_cast<USize>(Format::R16G16B16_SScaled)] = VK_FORMAT_R16G16B16_SSCALED;
        result[static_cast<USize>(Format::R16G16B16_UInt)] = VK_FORMAT_R16G16B16_UINT;
        result[static_cast<USize>(Format::R16G16B16_SInt)] = VK_FORMAT_R16G16B16_SINT;
        result[static_cast<USize>(Format::R16G16B16_SFloat)] = VK_FORMAT_R16G16B16_SFLOAT;
        result[static_cast<USize>(Format::R16G16B16A16_UNorm)] = VK_FORMAT_R16G16B16A16_UNORM;
        result[static_cast<USize>(Format::R16G16B16A16_SNorm)] = VK_FORMAT_R16G16B16A16_SNORM;
        result[static_cast<USize>(Format::R16G16B16A16_UScaled)] = VK_FORMAT_R16G16B16A16_USCALED;
        result[static_cast<USize>(Format::R16G16B16A16_SScaled)] = VK_FORMAT_R16G16B16A16_SSCALED;
        result[static_cast<USize>(Format::R16G16B16A16_UInt)] = VK_FORMAT_R16G16B16A16_UINT;
        result[static_cast<USize>(Format::R16G16B16A16_SInt)] = VK_FORMAT_R16G16B16A16_SINT;
        result[static_cast<USize>(Format::R16G16B16A16_SFloat)] = VK_FORMAT_R16G16B16A16_SFLOAT;
        result[static_cast<USize>(Format::R32_UInt)] = VK_FORMAT_R32_UINT;
        result[static_cast<USize>(Format::R32_SInt)] = VK_FORMAT_R32_SINT;
        result[static_cast<USize>(Format::R32_SFloat)] = VK_FORMAT_R32_SFLOAT;
        result[static_cast<USize>(Format::R32G32_UInt)] = VK_FORMAT_R32G32_UINT;
        result[static_cast<USize>(Format::R32G32_SInt)] = VK_FORMAT_R32G32_SINT;
        result[static_cast<USize>(Format::R32G32_SFloat)] = VK_FORMAT_R32G32_SFLOAT;
        result[static_cast<USize>(Format::R32G32B32_UInt)] = VK_FORMAT_R32G32B32_UINT;
        result[static_cast<USize>(Format::R32G32B32_SInt)] = VK_FORMAT_R32G32B32_SINT;
        result[static_cast<USize>(Format::R32G32B32_SFloat)] = VK_FORMAT_R32G32B32_SFLOAT;
        result[static_cast<USize>(Format::R32G32B32A32_UInt)] = VK_FORMAT_R32G32B32A32_UINT;
        result[static_cast<USize>(Format::R32G32B32A32_SInt)] = VK_FORMAT_R32G32B32A32_SINT;
        result[static_cast<USize>(Format::R32G32B32A32_SFloat)] = VK_FORMAT_R32G32B32A32_SFLOAT;
        result[static_cast<USize>(Format::R64_UInt)] = VK_FORMAT_R64_UINT;
        result[static_cast<USize>(Format::R64_SInt)] = VK_FORMAT_R64_SINT;
        result[static_cast<USize>(Format::R64_SFloat)] = VK_FORMAT_R64_SFLOAT;
        result[static_cast<USize>(Format::R64G64_UInt)] = VK_FORMAT_R64G64_UINT;
        result[static_cast<USize>(Format::R64G64_SInt)] = VK_FORMAT_R64G64_SINT;
        result[static_cast<USize>(Format::R64G64_SFloat)] = VK_FORMAT_R64G64_SFLOAT;
        result[static_cast<USize>(Format::R64G64B64_UInt)] = VK_FORMAT_R64G64B64_UINT;
        result[static_cast<USize>(Format::R64G64B64_SInt)] = VK_FORMAT_R64G64B64_SINT;
        result[static_cast<USize>(Format::R64G64B64_SFloat)] = VK_FORMAT_R64G64B64_SFLOAT;
        result[static_cast<USize>(Format::R64G64B64A64_UInt)] = VK_FORMAT_R64G64B64A64_UINT;
        result[static_cast<USize>(Format::R64G64B64A64_SInt)] = VK_FORMAT_R64G64B64A64_SINT;
        result[static_cast<USize>(Format::R64G64B64A64_SFloat)] = VK_FORMAT_R64G64B64A64_SFLOAT;
        result[static_cast<USize>(Format::B10G11R11_UFloat_Pack32)] = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        result[static_cast<USize>(Format::E5B9G9R9_UFloat_Pack32)] = VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
        result[static_cast<USize>(Format::D16_UNorm)] = VK_FORMAT_D16_UNORM;
        result[static_cast<USize>(Format::X8_D24_UNorm_Pack32)] = VK_FORMAT_X8_D24_UNORM_PACK32;
        result[static_cast<USize>(Format::D32_SFloat)] = VK_FORMAT_D32_SFLOAT;
        result[static_cast<USize>(Format::S8_UInt)] = VK_FORMAT_S8_UINT;
        result[static_cast<USize>(Format::D16_UNorm_S8_UInt)] = VK_FORMAT_D16_UNORM_S8_UINT;
        result[static_cast<USize>(Format::D24_UNorm_S8_UInt)] = VK_FORMAT_D24_UNORM_S8_UINT;
        result[static_cast<USize>(Format::D32_SFloat_S8_UInt)] = VK_FORMAT_D32_SFLOAT_S8_UINT;
        result[static_cast<USize>(Format::BC1_RGB_UNorm_Block)] = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        result[static_cast<USize>(Format::BC1_RGB_SRGB_Block)] = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
        result[static_cast<USize>(Format::BC1_RGBA_UNorm_Block)] = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        result[static_cast<USize>(Format::BC1_RGBA_SRGB_Block)] = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        result[static_cast<USize>(Format::BC2_UNorm_Block)] = VK_FORMAT_BC2_UNORM_BLOCK;
        result[static_cast<USize>(Format::BC2_SRGB_Block)] = VK_FORMAT_BC2_SRGB_BLOCK;
        result[static_cast<USize>(Format::BC3_UNorm_Block)] = VK_FORMAT_BC3_UNORM_BLOCK;
        result[static_cast<USize>(Format::BC3_SRGB_Block)] = VK_FORMAT_BC3_SRGB_BLOCK;
        result[static_cast<USize>(Format::BC4_UNorm_Block)] = VK_FORMAT_BC4_UNORM_BLOCK;
        result[static_cast<USize>(Format::BC4_SNorm_Block)] = VK_FORMAT_BC4_SNORM_BLOCK;
        result[static_cast<USize>(Format::BC5_UNorm_Block)] = VK_FORMAT_BC5_UNORM_BLOCK;
        result[static_cast<USize>(Format::BC5_SNorm_Block)] = VK_FORMAT_BC5_SNORM_BLOCK;
        result[static_cast<USize>(Format::BC6H_UFloat_Block)] = VK_FORMAT_BC6H_UFLOAT_BLOCK;
        result[static_cast<USize>(Format::BC6H_SFloat_Block)] = VK_FORMAT_BC6H_SFLOAT_BLOCK;
        result[static_cast<USize>(Format::BC7_UNorm_Block)] = VK_FORMAT_BC7_UNORM_BLOCK;
        result[static_cast<USize>(Format::BC7_SRGB_Block)] = VK_FORMAT_BC7_SRGB_BLOCK;
        result[static_cast<USize>(Format::ETC2_R8G8B8_UNorm_Block)] = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        result[static_cast<USize>(Format::ETC2_R8G8B8_SRGB_Block)] = VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
        result[static_cast<USize>(Format::ETC2_R8G8B8A1_UNorm_Block)] = VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
        result[static_cast<USize>(Format::ETC2_R8G8B8A1_SRGB_Block)] = VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
        result[static_cast<USize>(Format::ETC2_R8G8B8A8_UNorm_Block)] = VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        result[static_cast<USize>(Format::ETC2_R8G8B8A8_SRGB_Block)] = VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
        result[static_cast<USize>(Format::EAC_R11_UNorm_Block)] = VK_FORMAT_EAC_R11_UNORM_BLOCK;
        result[static_cast<USize>(Format::EAC_R11_SNorm_Block)] = VK_FORMAT_EAC_R11_SNORM_BLOCK;
        result[static_cast<USize>(Format::EAC_R11G11_UNorm_Block)] = VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
        result[static_cast<USize>(Format::EAC_R11G11_SNorm_Block)] = VK_FORMAT_EAC_R11G11_SNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_4x4_UNorm_Block)] = VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_4x4_SRGB_Block)] = VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_5x4_UNorm_Block)] = VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_5x4_SRGB_Block)] = VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_5x5_UNorm_Block)] = VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_5x5_SRGB_Block)] = VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_6x5_UNorm_Block)] = VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_6x5_SRGB_Block)] = VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_6x6_UNorm_Block)] = VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_6x6_SRGB_Block)] = VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_8x5_UNorm_Block)] = VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_8x5_SRGB_Block)] = VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_8x6_UNorm_Block)] = VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_8x6_SRGB_Block)] = VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_8x8_UNorm_Block)] = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_8x8_SRGB_Block)] = VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_10x5_UNorm_Block)] = VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_10x5_SRGB_Block)] = VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_10x6_UNorm_Block)] = VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_10x6_SRGB_Block)] = VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_10x8_UNorm_Block)] = VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_10x8_SRGB_Block)] = VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_10x10_UNorm_Block)] = VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_10x10_SRGB_Block)] = VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_12x10_UNorm_Block)] = VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_12x10_SRGB_Block)] = VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
        result[static_cast<USize>(Format::ASTC_12x12_UNorm_Block)] = VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
        result[static_cast<USize>(Format::ASTC_12x12_SRGB_Block)] = VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
        result[static_cast<USize>(Format::G8B8G8R8_422_UNorm)] = VK_FORMAT_G8B8G8R8_422_UNORM;
        result[static_cast<USize>(Format::B8G8R8G8_422_UNorm)] = VK_FORMAT_B8G8R8G8_422_UNORM;
        result[static_cast<USize>(Format::G8_B8_R8_3PLANE_420_UNorm)] = VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
        result[static_cast<USize>(Format::G8_B8R8_2PLANE_420_UNorm)] = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
        result[static_cast<USize>(Format::G8_B8_R8_3PLANE_422_UNorm)] = VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM;
        result[static_cast<USize>(Format::G8_B8R8_2PLANE_422_UNorm)] = VK_FORMAT_G8_B8R8_2PLANE_422_UNORM;
        result[static_cast<USize>(Format::G8_B8_R8_3PLANE_444_UNorm)] = VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM;
        result[static_cast<USize>(Format::R10X6_UNorm_Pack16)] = VK_FORMAT_R10X6_UNORM_PACK16;
        result[static_cast<USize>(Format::R10X6G10X6_UNorm_2Pack16)] = VK_FORMAT_R10X6G10X6_UNORM_2PACK16;
        result[static_cast<USize>(Format::R10X6G10X6B10X6A10X6_UNorm_4Pack16)] = VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16;
        result[static_cast<USize>(Format::G10X6B10X6G10X6R10X6_422_UNorm_4Pack16)] = VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16;
        result[static_cast<USize>(Format::B10X6G10X6R10X6G10X6_422_UNorm_4Pack16)] = VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16;
        result[static_cast<USize>(Format::G10X6_B10X6_R10X6_3PLANE_420_UNorm_3Pack16)] = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;
        result[static_cast<USize>(Format::G10X6_B10X6R10X6_2PLANE_420_UNorm_3Pack16)] = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
        result[static_cast<USize>(Format::G10X6_B10X6_R10X6_3PLANE_422_UNorm_3Pack16)] = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16;
        result[static_cast<USize>(Format::G10X6_B10X6R10X6_2PLANE_422_UNorm_3Pack16)] = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16;
        result[static_cast<USize>(Format::G10X6_B10X6_R10X6_3PLANE_444_UNorm_3Pack16)] = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16;
        result[static_cast<USize>(Format::R12X4_UNorm_Pack16)] = VK_FORMAT_R12X4_UNORM_PACK16;
        result[static_cast<USize>(Format::R12X4G12X4_UNorm_2Pack16)] = VK_FORMAT_R12X4G12X4_UNORM_2PACK16;
        result[static_cast<USize>(Format::R12X4G12X4B12X4A12X4_UNorm_4Pack16)] = VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16;
        result[static_cast<USize>(Format::G12X4B12X4G12X4R12X4_422_UNorm_4Pack16)] = VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16;
        result[static_cast<USize>(Format::B12X4G12X4R12X4G12X4_422_UNorm_4Pack16)] = VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16;
        result[static_cast<USize>(Format::G12X4_B12X4_R12X4_3PLANE_420_UNorm_3Pack16)] = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16;
        result[static_cast<USize>(Format::G12X4_B12X4R12X4_2PLANE_420_UNorm_3Pack16)] = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16;
        result[static_cast<USize>(Format::G12X4_B12X4_R12X4_3PLANE_422_UNorm_3Pack16)] = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16;
        result[static_cast<USize>(Format::G12X4_B12X4R12X4_2PLANE_422_UNorm_3Pack16)] = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16;
        result[static_cast<USize>(Format::G12X4_B12X4_R12X4_3PLANE_444_UNorm_3Pack16)] = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16;
        result[static_cast<USize>(Format::G16B16G16R16_422_UNorm)] = VK_FORMAT_G16B16G16R16_422_UNORM;
        result[static_cast<USize>(Format::B16G16R16G16_422_UNorm)] = VK_FORMAT_B16G16R16G16_422_UNORM;
        result[static_cast<USize>(Format::G16_B16_R16_3PLANE_420_UNorm)] = VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM;
        result[static_cast<USize>(Format::G16_B16R16_2PLANE_420_UNorm)] = VK_FORMAT_G16_B16R16_2PLANE_420_UNORM;
        result[static_cast<USize>(Format::G16_B16_R16_3PLANE_422_UNorm)] = VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM;
        result[static_cast<USize>(Format::G16_B16R16_2PLANE_422_UNorm)] = VK_FORMAT_G16_B16R16_2PLANE_422_UNORM;
        result[static_cast<USize>(Format::G16_B16_R16_3PLANE_444_UNorm)] = VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM;
        result[static_cast<USize>(Format::PVRTC1_2BPP_UNorm_Block_IMG)] = VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
        result[static_cast<USize>(Format::PVRTC1_4BPP_UNorm_Block_IMG)] = VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
        result[static_cast<USize>(Format::PVRTC2_2BPP_UNorm_Block_IMG)] = VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG;
        result[static_cast<USize>(Format::PVRTC2_4BPP_UNorm_Block_IMG)] = VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;
        result[static_cast<USize>(Format::PVRTC1_2BPP_SRGB_Block_IMG)] = VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG;
        result[static_cast<USize>(Format::PVRTC1_4BPP_SRGB_Block_IMG)] = VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG;
        result[static_cast<USize>(Format::PVRTC2_2BPP_SRGB_Block_IMG)] = VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG;
        result[static_cast<USize>(Format::PVRTC2_4BPP_SRGB_Block_IMG)] = VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG;
        result[static_cast<USize>(Format::ASTC_4x4_SFloat_Block_EXT)] = VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_5x4_SFloat_Block_EXT)] = VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_5x5_SFloat_Block_EXT)] = VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_6x5_SFloat_Block_EXT)] = VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_6x6_SFloat_Block_EXT)] = VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_8x5_SFloat_Block_EXT)] = VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_8x6_SFloat_Block_EXT)] = VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_8x8_SFloat_Block_EXT)] = VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_10x5_SFloat_Block_EXT)] = VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_10x6_SFloat_Block_EXT)] = VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_10x8_SFloat_Block_EXT)] = VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_10x10_SFloat_Block_EXT)] = VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_12x10_SFloat_Block_EXT)] = VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::ASTC_12x12_SFloat_Block_EXT)] = VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT;
        result[static_cast<USize>(Format::G8_B8R8_2PLANE_444_UNorm_EXT)] = VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT;
        result[static_cast<USize>(Format::G10X6_B10X6R10X6_2PLANE_444_UNorm_3Pack16_EXT)] = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT;
        result[static_cast<USize>(Format::G12X4_B12X4R12X4_2PLANE_444_UNorm_3Pack16_EXT)] = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT;
        result[static_cast<USize>(Format::G16_B16R16_2PLANE_444_UNorm_EXT)] = VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT;
        result[static_cast<USize>(Format::A4R4G4B4_UNorm_Pack16_EXT)] = VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT;
        result[static_cast<USize>(Format::A4B4G4R4_UNorm_Pack16_EXT)] = VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT;
        result[static_cast<USize>(Format::B10X6G10X6R10X6G10X6_422_UNorm_4Pack16_KHR)] = VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR;
        result[static_cast<USize>(Format::B12X4G12X4R12X4G12X4_422_UNorm_4Pack16_KHR)] = VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR;
        result[static_cast<USize>(Format::B16G16R16G16_422_UNorm_KHR)] = VK_FORMAT_B16G16R16G16_422_UNORM_KHR;
        result[static_cast<USize>(Format::B8G8R8G8_422_UNorm_KHR)] = VK_FORMAT_B8G8R8G8_422_UNORM_KHR;
        result[static_cast<USize>(Format::G10X6B10X6G10X6R10X6_422_UNorm_4Pack16_KHR)] = VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR;
        result[static_cast<USize>(Format::G10X6_B10X6R10X6_2PLANE_420_UNorm_3Pack16_KHR)] = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR;
        result[static_cast<USize>(Format::G10X6_B10X6R10X6_2PLANE_422_UNorm_3Pack16_KHR)] = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR;
        result[static_cast<USize>(Format::G10X6_B10X6_R10X6_3PLANE_420_UNorm_3Pack16_KHR)] = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR;
        result[static_cast<USize>(Format::G10X6_B10X6_R10X6_3PLANE_422_UNorm_3Pack16_KHR)] = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR;
        result[static_cast<USize>(Format::G10X6_B10X6_R10X6_3PLANE_444_UNorm_3Pack16_KHR)] = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR;
        result[static_cast<USize>(Format::G12X4B12X4G12X4R12X4_422_UNorm_4Pack16_KHR)] = VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR;
        result[static_cast<USize>(Format::G12X4_B12X4R12X4_2PLANE_420_UNorm_3Pack16_KHR)] = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR;
        result[static_cast<USize>(Format::G12X4_B12X4R12X4_2PLANE_422_UNorm_3Pack16_KHR)] = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR;
        result[static_cast<USize>(Format::G12X4_B12X4_R12X4_3PLANE_420_UNorm_3Pack16_KHR)] = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR;
        result[static_cast<USize>(Format::G12X4_B12X4_R12X4_3PLANE_422_UNorm_3Pack16_KHR)] = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR;
        result[static_cast<USize>(Format::G12X4_B12X4_R12X4_3PLANE_444_UNorm_3Pack16_KHR)] = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR;
        result[static_cast<USize>(Format::G16B16G16R16_422_UNorm_KHR)] = VK_FORMAT_G16B16G16R16_422_UNORM_KHR;
        result[static_cast<USize>(Format::G16_B16R16_2PLANE_420_UNorm_KHR)] = VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR;
        result[static_cast<USize>(Format::G16_B16R16_2PLANE_422_UNorm_KHR)] = VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR;
        result[static_cast<USize>(Format::G16_B16_R16_3PLANE_420_UNorm_KHR)] = VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR;
        result[static_cast<USize>(Format::G16_B16_R16_3PLANE_422_UNorm_KHR)] = VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR;
        result[static_cast<USize>(Format::G16_B16_R16_3PLANE_444_UNorm_KHR)] = VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR;
        result[static_cast<USize>(Format::G8B8G8R8_422_UNorm_KHR)] = VK_FORMAT_G8B8G8R8_422_UNORM_KHR;
        result[static_cast<USize>(Format::G8_B8R8_2PLANE_420_UNorm_KHR)] = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR;
        result[static_cast<USize>(Format::G8_B8R8_2PLANE_422_UNorm_KHR)] = VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR;
        result[static_cast<USize>(Format::G8_B8_R8_3PLANE_420_UNorm_KHR)] = VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR;
        result[static_cast<USize>(Format::G8_B8_R8_3PLANE_422_UNorm_KHR)] = VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR;
        result[static_cast<USize>(Format::G8_B8_R8_3PLANE_444_UNorm_KHR)] = VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR;
        result[static_cast<USize>(Format::R10X6G10X6B10X6A10X6_UNorm_4Pack16_KHR)] = VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR;
        result[static_cast<USize>(Format::R10X6G10X6_UNorm_2Pack16_KHR)] = VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR;
        result[static_cast<USize>(Format::R10X6_UNorm_Pack16_KHR)] = VK_FORMAT_R10X6_UNORM_PACK16_KHR;
        result[static_cast<USize>(Format::R12X4G12X4B12X4A12X4_UNorm_4Pack16_KHR)] = VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR;
        result[static_cast<USize>(Format::R12X4G12X4_UNorm_2Pack16_KHR)] = VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR;
        result[static_cast<USize>(Format::R12X4_UNorm_Pack16_KHR)] = VK_FORMAT_R12X4_UNORM_PACK16_KHR;
        return result;
    }();
    // clang-format on

    // O(1)
    inline VkFormat VKConvert(Format fmt)
    {
        return VKFormatConversions[static_cast<USize>(fmt)];
    }

    // O(N)
    inline Format VKConvert(VkFormat fmt)
    {
        // TODO: unordered_map<vk::Format, Format> ???
        // It is impossible to create the same array for backward conversions
        // because e.g. vk::Format::eR12X4UnormPack16KHR = 1'000'156'017
        for (USize i = 0; i < VKFormatConversions.size(); ++i)
        {
            if (fmt == VKFormatConversions[i])
            {
                return static_cast<Format>(i);
            }
        }

        return Format::None;
    }

    inline bool operator==(VkFormat lhs, Format rhs)
    {
        return lhs == VKConvert(rhs);
    }

    inline bool operator!=(VkFormat lhs, Format rhs)
    {
        return !(lhs == rhs);
    }

    inline bool operator==(Format lhs, VkFormat rhs)
    {
        return VKConvert(lhs) == rhs;
    }

    inline bool operator!=(Format lhs, VkFormat rhs)
    {
        return !(lhs == rhs);
    }
} // namespace FE::Osmium
