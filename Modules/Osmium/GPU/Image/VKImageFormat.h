#pragma once
#include <GPU/Common/VKConfig.h>
#include <GPU/Image/ImageFormat.h>
#include <array>
#include <tuple>

namespace FE::GPU
{
    // clang-format off
    static auto VKFormatConversions = []() {
        std::array<vk::Format, static_cast<size_t>(Format::Count)> result{};
        result[static_cast<size_t>(Format::None)] = vk::Format::eUndefined;
        result[static_cast<size_t>(Format::R4G4_UNorm_Pack8)] = vk::Format::eR4G4UnormPack8;
        result[static_cast<size_t>(Format::R4G4B4A4_UNorm_Pack16)] = vk::Format::eR4G4B4A4UnormPack16;
        result[static_cast<size_t>(Format::B4G4R4A4_UNorm_Pack16)] = vk::Format::eB4G4R4A4UnormPack16;
        result[static_cast<size_t>(Format::R5G6B5_UNorm_Pack16)] = vk::Format::eR5G6B5UnormPack16;
        result[static_cast<size_t>(Format::B5G6R5_UNorm_Pack16)] = vk::Format::eB5G6R5UnormPack16;
        result[static_cast<size_t>(Format::R5G5B5A1_UNorm_Pack16)] = vk::Format::eR5G5B5A1UnormPack16;
        result[static_cast<size_t>(Format::B5G5R5A1_UNorm_Pack16)] = vk::Format::eB5G5R5A1UnormPack16;
        result[static_cast<size_t>(Format::A1R5G5B5_UNorm_Pack16)] = vk::Format::eA1R5G5B5UnormPack16;
        result[static_cast<size_t>(Format::R8_UNorm)] = vk::Format::eR8Unorm;
        result[static_cast<size_t>(Format::R8_SNorm)] = vk::Format::eR8Snorm;
        result[static_cast<size_t>(Format::R8_UScaled)] = vk::Format::eR8Uscaled;
        result[static_cast<size_t>(Format::R8_SScaled)] = vk::Format::eR8Sscaled;
        result[static_cast<size_t>(Format::R8_UInt)] = vk::Format::eR8Uint;
        result[static_cast<size_t>(Format::R8_SInt)] = vk::Format::eR8Sint;
        result[static_cast<size_t>(Format::R8_SRGB)] = vk::Format::eR8Srgb;
        result[static_cast<size_t>(Format::R8G8_UNorm)] = vk::Format::eR8G8Unorm;
        result[static_cast<size_t>(Format::R8G8_SNorm)] = vk::Format::eR8G8Snorm;
        result[static_cast<size_t>(Format::R8G8_UScaled)] = vk::Format::eR8G8Uscaled;
        result[static_cast<size_t>(Format::R8G8_SScaled)] = vk::Format::eR8G8Sscaled;
        result[static_cast<size_t>(Format::R8G8_UInt)] = vk::Format::eR8G8Uint;
        result[static_cast<size_t>(Format::R8G8_SInt)] = vk::Format::eR8G8Sint;
        result[static_cast<size_t>(Format::R8G8_SRGB)] = vk::Format::eR8G8Srgb;
        result[static_cast<size_t>(Format::R8G8B8_UNorm)] = vk::Format::eR8G8B8Unorm;
        result[static_cast<size_t>(Format::R8G8B8_SNorm)] = vk::Format::eR8G8B8Snorm;
        result[static_cast<size_t>(Format::R8G8B8_UScaled)] = vk::Format::eR8G8B8Uscaled;
        result[static_cast<size_t>(Format::R8G8B8_SScaled)] = vk::Format::eR8G8B8Sscaled;
        result[static_cast<size_t>(Format::R8G8B8_UInt)] = vk::Format::eR8G8B8Uint;
        result[static_cast<size_t>(Format::R8G8B8_SInt)] = vk::Format::eR8G8B8Sint;
        result[static_cast<size_t>(Format::R8G8B8_SRGB)] = vk::Format::eR8G8B8Srgb;
        result[static_cast<size_t>(Format::B8G8R8_UNorm)] = vk::Format::eB8G8R8Unorm;
        result[static_cast<size_t>(Format::B8G8R8_SNorm)] = vk::Format::eB8G8R8Snorm;
        result[static_cast<size_t>(Format::B8G8R8_UScaled)] = vk::Format::eB8G8R8Uscaled;
        result[static_cast<size_t>(Format::B8G8R8_SScaled)] = vk::Format::eB8G8R8Sscaled;
        result[static_cast<size_t>(Format::B8G8R8_UInt)] = vk::Format::eB8G8R8Uint;
        result[static_cast<size_t>(Format::B8G8R8_SInt)] = vk::Format::eB8G8R8Sint;
        result[static_cast<size_t>(Format::B8G8R8_SRGB)] = vk::Format::eB8G8R8Srgb;
        result[static_cast<size_t>(Format::R8G8B8A8_UNorm)] = vk::Format::eR8G8B8A8Unorm;
        result[static_cast<size_t>(Format::R8G8B8A8_SNorm)] = vk::Format::eR8G8B8A8Snorm;
        result[static_cast<size_t>(Format::R8G8B8A8_UScaled)] = vk::Format::eR8G8B8A8Uscaled;
        result[static_cast<size_t>(Format::R8G8B8A8_SScaled)] = vk::Format::eR8G8B8A8Sscaled;
        result[static_cast<size_t>(Format::R8G8B8A8_UInt)] = vk::Format::eR8G8B8A8Uint;
        result[static_cast<size_t>(Format::R8G8B8A8_SInt)] = vk::Format::eR8G8B8A8Sint;
        result[static_cast<size_t>(Format::R8G8B8A8_SRGB)] = vk::Format::eR8G8B8A8Srgb;
        result[static_cast<size_t>(Format::B8G8R8A8_UNorm)] = vk::Format::eB8G8R8A8Unorm;
        result[static_cast<size_t>(Format::B8G8R8A8_SNorm)] = vk::Format::eB8G8R8A8Snorm;
        result[static_cast<size_t>(Format::B8G8R8A8_UScaled)] = vk::Format::eB8G8R8A8Uscaled;
        result[static_cast<size_t>(Format::B8G8R8A8_SScaled)] = vk::Format::eB8G8R8A8Sscaled;
        result[static_cast<size_t>(Format::B8G8R8A8_UInt)] = vk::Format::eB8G8R8A8Uint;
        result[static_cast<size_t>(Format::B8G8R8A8_SInt)] = vk::Format::eB8G8R8A8Sint;
        result[static_cast<size_t>(Format::B8G8R8A8_SRGB)] = vk::Format::eB8G8R8A8Srgb;
        result[static_cast<size_t>(Format::A8B8G8R8_UNorm_Pack32)] = vk::Format::eA8B8G8R8UnormPack32;
        result[static_cast<size_t>(Format::A8B8G8R8_SNorm_Pack32)] = vk::Format::eA8B8G8R8SnormPack32;
        result[static_cast<size_t>(Format::A8B8G8R8_UScaled_Pack32)] = vk::Format::eA8B8G8R8UscaledPack32;
        result[static_cast<size_t>(Format::A8B8G8R8_SScaled_Pack32)] = vk::Format::eA8B8G8R8SscaledPack32;
        result[static_cast<size_t>(Format::A8B8G8R8_UInt_Pack32)] = vk::Format::eA8B8G8R8UintPack32;
        result[static_cast<size_t>(Format::A8B8G8R8_SInt_Pack32)] = vk::Format::eA8B8G8R8SintPack32;
        result[static_cast<size_t>(Format::A8B8G8R8_SRGB_Pack32)] = vk::Format::eA8B8G8R8SrgbPack32;
        result[static_cast<size_t>(Format::A2R10G10B10_UNorm_Pack32)] = vk::Format::eA2R10G10B10UnormPack32;
        result[static_cast<size_t>(Format::A2R10G10B10_SNorm_Pack32)] = vk::Format::eA2R10G10B10SnormPack32;
        result[static_cast<size_t>(Format::A2R10G10B10_UScaled_Pack32)] = vk::Format::eA2R10G10B10UscaledPack32;
        result[static_cast<size_t>(Format::A2R10G10B10_SScaled_Pack32)] = vk::Format::eA2R10G10B10SscaledPack32;
        result[static_cast<size_t>(Format::A2R10G10B10_UInt_Pack32)] = vk::Format::eA2R10G10B10UintPack32;
        result[static_cast<size_t>(Format::A2R10G10B10_SInt_Pack32)] = vk::Format::eA2R10G10B10SintPack32;
        result[static_cast<size_t>(Format::A2B10G10R10_UNorm_Pack32)] = vk::Format::eA2B10G10R10UnormPack32;
        result[static_cast<size_t>(Format::A2B10G10R10_SNorm_Pack32)] = vk::Format::eA2B10G10R10SnormPack32;
        result[static_cast<size_t>(Format::A2B10G10R10_UScaled_Pack32)] = vk::Format::eA2B10G10R10UscaledPack32;
        result[static_cast<size_t>(Format::A2B10G10R10_SScaled_Pack32)] = vk::Format::eA2B10G10R10SscaledPack32;
        result[static_cast<size_t>(Format::A2B10G10R10_UInt_Pack32)] = vk::Format::eA2B10G10R10UintPack32;
        result[static_cast<size_t>(Format::A2B10G10R10_SInt_Pack32)] = vk::Format::eA2B10G10R10SintPack32;
        result[static_cast<size_t>(Format::R16_UNorm)] = vk::Format::eR16Unorm;
        result[static_cast<size_t>(Format::R16_SNorm)] = vk::Format::eR16Snorm;
        result[static_cast<size_t>(Format::R16_UScaled)] = vk::Format::eR16Uscaled;
        result[static_cast<size_t>(Format::R16_SScaled)] = vk::Format::eR16Sscaled;
        result[static_cast<size_t>(Format::R16_UInt)] = vk::Format::eR16Uint;
        result[static_cast<size_t>(Format::R16_SInt)] = vk::Format::eR16Sint;
        result[static_cast<size_t>(Format::R16_SFloat)] = vk::Format::eR16Sfloat;
        result[static_cast<size_t>(Format::R16G16_UNorm)] = vk::Format::eR16G16Unorm;
        result[static_cast<size_t>(Format::R16G16_SNorm)] = vk::Format::eR16G16Snorm;
        result[static_cast<size_t>(Format::R16G16_UScaled)] = vk::Format::eR16G16Uscaled;
        result[static_cast<size_t>(Format::R16G16_SScaled)] = vk::Format::eR16G16Sscaled;
        result[static_cast<size_t>(Format::R16G16_UInt)] = vk::Format::eR16G16Uint;
        result[static_cast<size_t>(Format::R16G16_SInt)] = vk::Format::eR16G16Sint;
        result[static_cast<size_t>(Format::R16G16_SFloat)] = vk::Format::eR16G16Sfloat;
        result[static_cast<size_t>(Format::R16G16B16_UNorm)] = vk::Format::eR16G16B16Unorm;
        result[static_cast<size_t>(Format::R16G16B16_SNorm)] = vk::Format::eR16G16B16Snorm;
        result[static_cast<size_t>(Format::R16G16B16_UScaled)] = vk::Format::eR16G16B16Uscaled;
        result[static_cast<size_t>(Format::R16G16B16_SScaled)] = vk::Format::eR16G16B16Sscaled;
        result[static_cast<size_t>(Format::R16G16B16_UInt)] = vk::Format::eR16G16B16Uint;
        result[static_cast<size_t>(Format::R16G16B16_SInt)] = vk::Format::eR16G16B16Sint;
        result[static_cast<size_t>(Format::R16G16B16_SFloat)] = vk::Format::eR16G16B16Sfloat;
        result[static_cast<size_t>(Format::R16G16B16A16_UNorm)] = vk::Format::eR16G16B16A16Unorm;
        result[static_cast<size_t>(Format::R16G16B16A16_SNorm)] = vk::Format::eR16G16B16A16Snorm;
        result[static_cast<size_t>(Format::R16G16B16A16_UScaled)] = vk::Format::eR16G16B16A16Uscaled;
        result[static_cast<size_t>(Format::R16G16B16A16_SScaled)] = vk::Format::eR16G16B16A16Sscaled;
        result[static_cast<size_t>(Format::R16G16B16A16_UInt)] = vk::Format::eR16G16B16A16Uint;
        result[static_cast<size_t>(Format::R16G16B16A16_SInt)] = vk::Format::eR16G16B16A16Sint;
        result[static_cast<size_t>(Format::R16G16B16A16_SFloat)] = vk::Format::eR16G16B16A16Sfloat;
        result[static_cast<size_t>(Format::R32_UInt)] = vk::Format::eR32Uint;
        result[static_cast<size_t>(Format::R32_SInt)] = vk::Format::eR32Sint;
        result[static_cast<size_t>(Format::R32_SFloat)] = vk::Format::eR32Sfloat;
        result[static_cast<size_t>(Format::R32G32_UInt)] = vk::Format::eR32G32Uint;
        result[static_cast<size_t>(Format::R32G32_SInt)] = vk::Format::eR32G32Sint;
        result[static_cast<size_t>(Format::R32G32_SFloat)] = vk::Format::eR32G32Sfloat;
        result[static_cast<size_t>(Format::R32G32B32_UInt)] = vk::Format::eR32G32B32Uint;
        result[static_cast<size_t>(Format::R32G32B32_SInt)] = vk::Format::eR32G32B32Sint;
        result[static_cast<size_t>(Format::R32G32B32_SFloat)] = vk::Format::eR32G32B32Sfloat;
        result[static_cast<size_t>(Format::R32G32B32A32_UInt)] = vk::Format::eR32G32B32A32Uint;
        result[static_cast<size_t>(Format::R32G32B32A32_SInt)] = vk::Format::eR32G32B32A32Sint;
        result[static_cast<size_t>(Format::R32G32B32A32_SFloat)] = vk::Format::eR32G32B32A32Sfloat;
        result[static_cast<size_t>(Format::R64_UInt)] = vk::Format::eR64Uint;
        result[static_cast<size_t>(Format::R64_SInt)] = vk::Format::eR64Sint;
        result[static_cast<size_t>(Format::R64_SFloat)] = vk::Format::eR64Sfloat;
        result[static_cast<size_t>(Format::R64G64_UInt)] = vk::Format::eR64G64Uint;
        result[static_cast<size_t>(Format::R64G64_SInt)] = vk::Format::eR64G64Sint;
        result[static_cast<size_t>(Format::R64G64_SFloat)] = vk::Format::eR64G64Sfloat;
        result[static_cast<size_t>(Format::R64G64B64_UInt)] = vk::Format::eR64G64B64Uint;
        result[static_cast<size_t>(Format::R64G64B64_SInt)] = vk::Format::eR64G64B64Sint;
        result[static_cast<size_t>(Format::R64G64B64_SFloat)] = vk::Format::eR64G64B64Sfloat;
        result[static_cast<size_t>(Format::R64G64B64A64_UInt)] = vk::Format::eR64G64B64A64Uint;
        result[static_cast<size_t>(Format::R64G64B64A64_SInt)] = vk::Format::eR64G64B64A64Sint;
        result[static_cast<size_t>(Format::R64G64B64A64_SFloat)] = vk::Format::eR64G64B64A64Sfloat;
        result[static_cast<size_t>(Format::B10G11R11_UFloat_Pack32)] = vk::Format::eB10G11R11UfloatPack32;
        result[static_cast<size_t>(Format::E5B9G9R9_UFloat_Pack32)] = vk::Format::eE5B9G9R9UfloatPack32;
        result[static_cast<size_t>(Format::D16_UNorm)] = vk::Format::eD16Unorm;
        result[static_cast<size_t>(Format::X8_D24_UNorm_Pack32)] = vk::Format::eX8D24UnormPack32;
        result[static_cast<size_t>(Format::D32_SFloat)] = vk::Format::eD32Sfloat;
        result[static_cast<size_t>(Format::S8_UInt)] = vk::Format::eS8Uint;
        result[static_cast<size_t>(Format::D16_UNorm_S8_UInt)] = vk::Format::eD16UnormS8Uint;
        result[static_cast<size_t>(Format::D24_UNorm_S8_UInt)] = vk::Format::eD24UnormS8Uint;
        result[static_cast<size_t>(Format::D32_SFloat_S8_UInt)] = vk::Format::eD32SfloatS8Uint;
        result[static_cast<size_t>(Format::BC1_RGB_UNorm_Block)] = vk::Format::eBc1RgbUnormBlock;
        result[static_cast<size_t>(Format::BC1_RGB_SRGB_Block)] = vk::Format::eBc1RgbSrgbBlock;
        result[static_cast<size_t>(Format::BC1_RGBA_UNorm_Block)] = vk::Format::eBc1RgbaUnormBlock;
        result[static_cast<size_t>(Format::BC1_RGBA_SRGB_Block)] = vk::Format::eBc1RgbaSrgbBlock;
        result[static_cast<size_t>(Format::BC2_UNorm_Block)] = vk::Format::eBc2UnormBlock;
        result[static_cast<size_t>(Format::BC2_SRGB_Block)] = vk::Format::eBc2SrgbBlock;
        result[static_cast<size_t>(Format::BC3_UNorm_Block)] = vk::Format::eBc3UnormBlock;
        result[static_cast<size_t>(Format::BC3_SRGB_Block)] = vk::Format::eBc3SrgbBlock;
        result[static_cast<size_t>(Format::BC4_UNorm_Block)] = vk::Format::eBc4UnormBlock;
        result[static_cast<size_t>(Format::BC4_SNorm_Block)] = vk::Format::eBc4SnormBlock;
        result[static_cast<size_t>(Format::BC5_UNorm_Block)] = vk::Format::eBc5UnormBlock;
        result[static_cast<size_t>(Format::BC5_SNorm_Block)] = vk::Format::eBc5SnormBlock;
        result[static_cast<size_t>(Format::BC6H_UFloat_Block)] = vk::Format::eBc6HUfloatBlock;
        result[static_cast<size_t>(Format::BC6H_SFloat_Block)] = vk::Format::eBc6HSfloatBlock;
        result[static_cast<size_t>(Format::BC7_UNorm_Block)] = vk::Format::eBc7UnormBlock;
        result[static_cast<size_t>(Format::BC7_SRGB_Block)] = vk::Format::eBc7SrgbBlock;
        result[static_cast<size_t>(Format::ETC2_R8G8B8_UNorm_Block)] = vk::Format::eEtc2R8G8B8UnormBlock;
        result[static_cast<size_t>(Format::ETC2_R8G8B8_SRGB_Block)] = vk::Format::eEtc2R8G8B8SrgbBlock;
        result[static_cast<size_t>(Format::ETC2_R8G8B8A1_UNorm_Block)] = vk::Format::eEtc2R8G8B8A1UnormBlock;
        result[static_cast<size_t>(Format::ETC2_R8G8B8A1_SRGB_Block)] = vk::Format::eEtc2R8G8B8A1SrgbBlock;
        result[static_cast<size_t>(Format::ETC2_R8G8B8A8_UNorm_Block)] = vk::Format::eEtc2R8G8B8A8UnormBlock;
        result[static_cast<size_t>(Format::ETC2_R8G8B8A8_SRGB_Block)] = vk::Format::eEtc2R8G8B8A8SrgbBlock;
        result[static_cast<size_t>(Format::EAC_R11_UNorm_Block)] = vk::Format::eEacR11UnormBlock;
        result[static_cast<size_t>(Format::EAC_R11_SNorm_Block)] = vk::Format::eEacR11SnormBlock;
        result[static_cast<size_t>(Format::EAC_R11G11_UNorm_Block)] = vk::Format::eEacR11G11UnormBlock;
        result[static_cast<size_t>(Format::EAC_R11G11_SNorm_Block)] = vk::Format::eEacR11G11SnormBlock;
        result[static_cast<size_t>(Format::ASTC_4x4_UNorm_Block)] = vk::Format::eAstc4x4UnormBlock;
        result[static_cast<size_t>(Format::ASTC_4x4_SRGB_Block)] = vk::Format::eAstc4x4SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_5x4_UNorm_Block)] = vk::Format::eAstc5x4UnormBlock;
        result[static_cast<size_t>(Format::ASTC_5x4_SRGB_Block)] = vk::Format::eAstc5x4SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_5x5_UNorm_Block)] = vk::Format::eAstc5x5UnormBlock;
        result[static_cast<size_t>(Format::ASTC_5x5_SRGB_Block)] = vk::Format::eAstc5x5SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_6x5_UNorm_Block)] = vk::Format::eAstc6x5UnormBlock;
        result[static_cast<size_t>(Format::ASTC_6x5_SRGB_Block)] = vk::Format::eAstc6x5SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_6x6_UNorm_Block)] = vk::Format::eAstc6x6UnormBlock;
        result[static_cast<size_t>(Format::ASTC_6x6_SRGB_Block)] = vk::Format::eAstc6x6SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_8x5_UNorm_Block)] = vk::Format::eAstc8x5UnormBlock;
        result[static_cast<size_t>(Format::ASTC_8x5_SRGB_Block)] = vk::Format::eAstc8x5SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_8x6_UNorm_Block)] = vk::Format::eAstc8x6UnormBlock;
        result[static_cast<size_t>(Format::ASTC_8x6_SRGB_Block)] = vk::Format::eAstc8x6SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_8x8_UNorm_Block)] = vk::Format::eAstc8x8UnormBlock;
        result[static_cast<size_t>(Format::ASTC_8x8_SRGB_Block)] = vk::Format::eAstc8x8SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_10x5_UNorm_Block)] = vk::Format::eAstc10x5UnormBlock;
        result[static_cast<size_t>(Format::ASTC_10x5_SRGB_Block)] = vk::Format::eAstc10x5SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_10x6_UNorm_Block)] = vk::Format::eAstc10x6UnormBlock;
        result[static_cast<size_t>(Format::ASTC_10x6_SRGB_Block)] = vk::Format::eAstc10x6SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_10x8_UNorm_Block)] = vk::Format::eAstc10x8UnormBlock;
        result[static_cast<size_t>(Format::ASTC_10x8_SRGB_Block)] = vk::Format::eAstc10x8SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_10x10_UNorm_Block)] = vk::Format::eAstc10x10UnormBlock;
        result[static_cast<size_t>(Format::ASTC_10x10_SRGB_Block)] = vk::Format::eAstc10x10SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_12x10_UNorm_Block)] = vk::Format::eAstc12x10UnormBlock;
        result[static_cast<size_t>(Format::ASTC_12x10_SRGB_Block)] = vk::Format::eAstc12x10SrgbBlock;
        result[static_cast<size_t>(Format::ASTC_12x12_UNorm_Block)] = vk::Format::eAstc12x12UnormBlock;
        result[static_cast<size_t>(Format::ASTC_12x12_SRGB_Block)] = vk::Format::eAstc12x12SrgbBlock;
        result[static_cast<size_t>(Format::G8B8G8R8_422_UNorm)] = vk::Format::eG8B8G8R8422Unorm;
        result[static_cast<size_t>(Format::B8G8R8G8_422_UNorm)] = vk::Format::eB8G8R8G8422Unorm;
        result[static_cast<size_t>(Format::G8_B8_R8_3PLANE_420_UNorm)] = vk::Format::eG8B8R83Plane420Unorm;
        result[static_cast<size_t>(Format::G8_B8R8_2PLANE_420_UNorm)] = vk::Format::eG8B8R82Plane420Unorm;
        result[static_cast<size_t>(Format::G8_B8_R8_3PLANE_422_UNorm)] = vk::Format::eG8B8R83Plane422Unorm;
        result[static_cast<size_t>(Format::G8_B8R8_2PLANE_422_UNorm)] = vk::Format::eG8B8R82Plane422Unorm;
        result[static_cast<size_t>(Format::G8_B8_R8_3PLANE_444_UNorm)] = vk::Format::eG8B8R83Plane444Unorm;
        result[static_cast<size_t>(Format::R10X6_UNorm_Pack16)] = vk::Format::eR10X6UnormPack16;
        result[static_cast<size_t>(Format::R10X6G10X6_UNorm_2Pack16)] = vk::Format::eR10X6G10X6Unorm2Pack16;
        result[static_cast<size_t>(Format::R10X6G10X6B10X6A10X6_UNorm_4Pack16)] = vk::Format::eR10X6G10X6B10X6A10X6Unorm4Pack16;
        result[static_cast<size_t>(Format::G10X6B10X6G10X6R10X6_422_UNorm_4Pack16)] = vk::Format::eG10X6B10X6G10X6R10X6422Unorm4Pack16;
        result[static_cast<size_t>(Format::B10X6G10X6R10X6G10X6_422_UNorm_4Pack16)] = vk::Format::eB10X6G10X6R10X6G10X6422Unorm4Pack16;
        result[static_cast<size_t>(Format::G10X6_B10X6_R10X6_3PLANE_420_UNorm_3Pack16)] = vk::Format::eG10X6B10X6R10X63Plane420Unorm3Pack16;
        result[static_cast<size_t>(Format::G10X6_B10X6R10X6_2PLANE_420_UNorm_3Pack16)] = vk::Format::eG10X6B10X6R10X62Plane420Unorm3Pack16;
        result[static_cast<size_t>(Format::G10X6_B10X6_R10X6_3PLANE_422_UNorm_3Pack16)] = vk::Format::eG10X6B10X6R10X63Plane422Unorm3Pack16;
        result[static_cast<size_t>(Format::G10X6_B10X6R10X6_2PLANE_422_UNorm_3Pack16)] = vk::Format::eG10X6B10X6R10X62Plane422Unorm3Pack16;
        result[static_cast<size_t>(Format::G10X6_B10X6_R10X6_3PLANE_444_UNorm_3Pack16)] = vk::Format::eG10X6B10X6R10X63Plane444Unorm3Pack16;
        result[static_cast<size_t>(Format::R12X4_UNorm_Pack16)] = vk::Format::eR12X4UnormPack16;
        result[static_cast<size_t>(Format::R12X4G12X4_UNorm_2Pack16)] = vk::Format::eR12X4G12X4Unorm2Pack16;
        result[static_cast<size_t>(Format::R12X4G12X4B12X4A12X4_UNorm_4Pack16)] = vk::Format::eR12X4G12X4B12X4A12X4Unorm4Pack16;
        result[static_cast<size_t>(Format::G12X4B12X4G12X4R12X4_422_UNorm_4Pack16)] = vk::Format::eG12X4B12X4G12X4R12X4422Unorm4Pack16;
        result[static_cast<size_t>(Format::B12X4G12X4R12X4G12X4_422_UNorm_4Pack16)] = vk::Format::eB12X4G12X4R12X4G12X4422Unorm4Pack16;
        result[static_cast<size_t>(Format::G12X4_B12X4_R12X4_3PLANE_420_UNorm_3Pack16)] = vk::Format::eG12X4B12X4R12X43Plane420Unorm3Pack16;
        result[static_cast<size_t>(Format::G12X4_B12X4R12X4_2PLANE_420_UNorm_3Pack16)] = vk::Format::eG12X4B12X4R12X42Plane420Unorm3Pack16;
        result[static_cast<size_t>(Format::G12X4_B12X4_R12X4_3PLANE_422_UNorm_3Pack16)] = vk::Format::eG12X4B12X4R12X43Plane422Unorm3Pack16;
        result[static_cast<size_t>(Format::G12X4_B12X4R12X4_2PLANE_422_UNorm_3Pack16)] = vk::Format::eG12X4B12X4R12X42Plane422Unorm3Pack16;
        result[static_cast<size_t>(Format::G12X4_B12X4_R12X4_3PLANE_444_UNorm_3Pack16)] = vk::Format::eG12X4B12X4R12X43Plane444Unorm3Pack16;
        result[static_cast<size_t>(Format::G16B16G16R16_422_UNorm)] = vk::Format::eG16B16G16R16422Unorm;
        result[static_cast<size_t>(Format::B16G16R16G16_422_UNorm)] = vk::Format::eB16G16R16G16422Unorm;
        result[static_cast<size_t>(Format::G16_B16_R16_3PLANE_420_UNorm)] = vk::Format::eG16B16R163Plane420Unorm;
        result[static_cast<size_t>(Format::G16_B16R16_2PLANE_420_UNorm)] = vk::Format::eG16B16R162Plane420Unorm;
        result[static_cast<size_t>(Format::G16_B16_R16_3PLANE_422_UNorm)] = vk::Format::eG16B16R163Plane422Unorm;
        result[static_cast<size_t>(Format::G16_B16R16_2PLANE_422_UNorm)] = vk::Format::eG16B16R162Plane422Unorm;
        result[static_cast<size_t>(Format::G16_B16_R16_3PLANE_444_UNorm)] = vk::Format::eG16B16R163Plane444Unorm;
        result[static_cast<size_t>(Format::PVRTC1_2BPP_UNorm_Block_IMG)] = vk::Format::ePvrtc12BppUnormBlockIMG;
        result[static_cast<size_t>(Format::PVRTC1_4BPP_UNorm_Block_IMG)] = vk::Format::ePvrtc14BppUnormBlockIMG;
        result[static_cast<size_t>(Format::PVRTC2_2BPP_UNorm_Block_IMG)] = vk::Format::ePvrtc22BppUnormBlockIMG;
        result[static_cast<size_t>(Format::PVRTC2_4BPP_UNorm_Block_IMG)] = vk::Format::ePvrtc24BppUnormBlockIMG;
        result[static_cast<size_t>(Format::PVRTC1_2BPP_SRGB_Block_IMG)] = vk::Format::ePvrtc12BppSrgbBlockIMG;
        result[static_cast<size_t>(Format::PVRTC1_4BPP_SRGB_Block_IMG)] = vk::Format::ePvrtc14BppSrgbBlockIMG;
        result[static_cast<size_t>(Format::PVRTC2_2BPP_SRGB_Block_IMG)] = vk::Format::ePvrtc22BppSrgbBlockIMG;
        result[static_cast<size_t>(Format::PVRTC2_4BPP_SRGB_Block_IMG)] = vk::Format::ePvrtc24BppSrgbBlockIMG;
        result[static_cast<size_t>(Format::ASTC_4x4_SFloat_Block_EXT)] = vk::Format::eAstc4x4SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_5x4_SFloat_Block_EXT)] = vk::Format::eAstc5x4SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_5x5_SFloat_Block_EXT)] = vk::Format::eAstc5x5SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_6x5_SFloat_Block_EXT)] = vk::Format::eAstc6x5SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_6x6_SFloat_Block_EXT)] = vk::Format::eAstc6x6SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_8x5_SFloat_Block_EXT)] = vk::Format::eAstc8x5SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_8x6_SFloat_Block_EXT)] = vk::Format::eAstc8x6SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_8x8_SFloat_Block_EXT)] = vk::Format::eAstc8x8SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_10x5_SFloat_Block_EXT)] = vk::Format::eAstc10x5SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_10x6_SFloat_Block_EXT)] = vk::Format::eAstc10x6SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_10x8_SFloat_Block_EXT)] = vk::Format::eAstc10x8SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_10x10_SFloat_Block_EXT)] = vk::Format::eAstc10x10SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_12x10_SFloat_Block_EXT)] = vk::Format::eAstc12x10SfloatBlockEXT;
        result[static_cast<size_t>(Format::ASTC_12x12_SFloat_Block_EXT)] = vk::Format::eAstc12x12SfloatBlockEXT;
        result[static_cast<size_t>(Format::G8_B8R8_2PLANE_444_UNorm_EXT)] = vk::Format::eG8B8R82Plane444UnormEXT;
        result[static_cast<size_t>(Format::G10X6_B10X6R10X6_2PLANE_444_UNorm_3Pack16_EXT)] = vk::Format::eG10X6B10X6R10X62Plane444Unorm3Pack16EXT;
        result[static_cast<size_t>(Format::G12X4_B12X4R12X4_2PLANE_444_UNorm_3Pack16_EXT)] = vk::Format::eG12X4B12X4R12X42Plane444Unorm3Pack16EXT;
        result[static_cast<size_t>(Format::G16_B16R16_2PLANE_444_UNorm_EXT)] = vk::Format::eG16B16R162Plane444UnormEXT;
        result[static_cast<size_t>(Format::A4R4G4B4_UNorm_Pack16_EXT)] = vk::Format::eA4R4G4B4UnormPack16EXT;
        result[static_cast<size_t>(Format::A4B4G4R4_UNorm_Pack16_EXT)] = vk::Format::eA4B4G4R4UnormPack16EXT;
        result[static_cast<size_t>(Format::B10X6G10X6R10X6G10X6_422_UNorm_4Pack16_KHR)] = vk::Format::eB10X6G10X6R10X6G10X6422Unorm4Pack16KHR;
        result[static_cast<size_t>(Format::B12X4G12X4R12X4G12X4_422_UNorm_4Pack16_KHR)] = vk::Format::eB12X4G12X4R12X4G12X4422Unorm4Pack16KHR;
        result[static_cast<size_t>(Format::B16G16R16G16_422_UNorm_KHR)] = vk::Format::eB16G16R16G16422UnormKHR;
        result[static_cast<size_t>(Format::B8G8R8G8_422_UNorm_KHR)] = vk::Format::eB8G8R8G8422UnormKHR;
        result[static_cast<size_t>(Format::G10X6B10X6G10X6R10X6_422_UNorm_4Pack16_KHR)] = vk::Format::eG10X6B10X6G10X6R10X6422Unorm4Pack16KHR;
        result[static_cast<size_t>(Format::G10X6_B10X6R10X6_2PLANE_420_UNorm_3Pack16_KHR)] = vk::Format::eG10X6B10X6R10X62Plane420Unorm3Pack16KHR;
        result[static_cast<size_t>(Format::G10X6_B10X6R10X6_2PLANE_422_UNorm_3Pack16_KHR)] = vk::Format::eG10X6B10X6R10X62Plane422Unorm3Pack16KHR;
        result[static_cast<size_t>(Format::G10X6_B10X6_R10X6_3PLANE_420_UNorm_3Pack16_KHR)] = vk::Format::eG10X6B10X6R10X63Plane420Unorm3Pack16KHR;
        result[static_cast<size_t>(Format::G10X6_B10X6_R10X6_3PLANE_422_UNorm_3Pack16_KHR)] = vk::Format::eG10X6B10X6R10X63Plane422Unorm3Pack16KHR;
        result[static_cast<size_t>(Format::G10X6_B10X6_R10X6_3PLANE_444_UNorm_3Pack16_KHR)] = vk::Format::eG10X6B10X6R10X63Plane444Unorm3Pack16KHR;
        result[static_cast<size_t>(Format::G12X4B12X4G12X4R12X4_422_UNorm_4Pack16_KHR)] = vk::Format::eG12X4B12X4G12X4R12X4422Unorm4Pack16KHR;
        result[static_cast<size_t>(Format::G12X4_B12X4R12X4_2PLANE_420_UNorm_3Pack16_KHR)] = vk::Format::eG12X4B12X4R12X42Plane420Unorm3Pack16KHR;
        result[static_cast<size_t>(Format::G12X4_B12X4R12X4_2PLANE_422_UNorm_3Pack16_KHR)] = vk::Format::eG12X4B12X4R12X42Plane422Unorm3Pack16KHR;
        result[static_cast<size_t>(Format::G12X4_B12X4_R12X4_3PLANE_420_UNorm_3Pack16_KHR)] = vk::Format::eG12X4B12X4R12X43Plane420Unorm3Pack16KHR;
        result[static_cast<size_t>(Format::G12X4_B12X4_R12X4_3PLANE_422_UNorm_3Pack16_KHR)] = vk::Format::eG12X4B12X4R12X43Plane422Unorm3Pack16KHR;
        result[static_cast<size_t>(Format::G12X4_B12X4_R12X4_3PLANE_444_UNorm_3Pack16_KHR)] = vk::Format::eG12X4B12X4R12X43Plane444Unorm3Pack16KHR;
        result[static_cast<size_t>(Format::G16B16G16R16_422_UNorm_KHR)] = vk::Format::eG16B16G16R16422UnormKHR;
        result[static_cast<size_t>(Format::G16_B16R16_2PLANE_420_UNorm_KHR)] = vk::Format::eG16B16R162Plane420UnormKHR;
        result[static_cast<size_t>(Format::G16_B16R16_2PLANE_422_UNorm_KHR)] = vk::Format::eG16B16R162Plane422UnormKHR;
        result[static_cast<size_t>(Format::G16_B16_R16_3PLANE_420_UNorm_KHR)] = vk::Format::eG16B16R163Plane420UnormKHR;
        result[static_cast<size_t>(Format::G16_B16_R16_3PLANE_422_UNorm_KHR)] = vk::Format::eG16B16R163Plane422UnormKHR;
        result[static_cast<size_t>(Format::G16_B16_R16_3PLANE_444_UNorm_KHR)] = vk::Format::eG16B16R163Plane444UnormKHR;
        result[static_cast<size_t>(Format::G8B8G8R8_422_UNorm_KHR)] = vk::Format::eG8B8G8R8422UnormKHR;
        result[static_cast<size_t>(Format::G8_B8R8_2PLANE_420_UNorm_KHR)] = vk::Format::eG8B8R82Plane420UnormKHR;
        result[static_cast<size_t>(Format::G8_B8R8_2PLANE_422_UNorm_KHR)] = vk::Format::eG8B8R82Plane422UnormKHR;
        result[static_cast<size_t>(Format::G8_B8_R8_3PLANE_420_UNorm_KHR)] = vk::Format::eG8B8R83Plane420UnormKHR;
        result[static_cast<size_t>(Format::G8_B8_R8_3PLANE_422_UNorm_KHR)] = vk::Format::eG8B8R83Plane422UnormKHR;
        result[static_cast<size_t>(Format::G8_B8_R8_3PLANE_444_UNorm_KHR)] = vk::Format::eG8B8R83Plane444UnormKHR;
        result[static_cast<size_t>(Format::R10X6G10X6B10X6A10X6_UNorm_4Pack16_KHR)] = vk::Format::eR10X6G10X6B10X6A10X6Unorm4Pack16KHR;
        result[static_cast<size_t>(Format::R10X6G10X6_UNorm_2Pack16_KHR)] = vk::Format::eR10X6G10X6Unorm2Pack16KHR;
        result[static_cast<size_t>(Format::R10X6_UNorm_Pack16_KHR)] = vk::Format::eR10X6UnormPack16KHR;
        result[static_cast<size_t>(Format::R12X4G12X4B12X4A12X4_UNorm_4Pack16_KHR)] = vk::Format::eR12X4G12X4B12X4A12X4Unorm4Pack16KHR;
        result[static_cast<size_t>(Format::R12X4G12X4_UNorm_2Pack16_KHR)] = vk::Format::eR12X4G12X4Unorm2Pack16KHR;
        result[static_cast<size_t>(Format::R12X4_UNorm_Pack16_KHR)] = vk::Format::eR12X4UnormPack16KHR;
        return result;
    }();
    // clang-format on

    // O(1)
    inline vk::Format VKConvert(Format fmt)
    {
        return VKFormatConversions[static_cast<size_t>(fmt)];
    }

    // O(N)
    inline Format VKConvert(vk::Format fmt)
    {
        // TODO: unordered_map<vk::Format, Format> ???
        // It is impossible to create the same array for backward conversions
        // because e.g. vk::Format::eR12X4UnormPack16KHR = 1'000'156'017
        for (size_t i = 0; i < VKFormatConversions.size(); ++i)
        {
            if (fmt == VKFormatConversions[i])
            {
                return static_cast<Format>(i);
            }
        }

        return Format::None;
    }

    inline bool operator==(vk::Format lhs, Format rhs)
    {
        return lhs == VKConvert(rhs);
    }

    inline bool operator!=(vk::Format lhs, Format rhs)
    {
        return !(lhs == rhs);
    }

    inline bool operator==(Format lhs, vk::Format rhs)
    {
        return VKConvert(lhs) == rhs;
    }

    inline bool operator!=(Format lhs, vk::Format rhs)
    {
        return !(lhs == rhs);
    }
} // namespace FE::GPU
