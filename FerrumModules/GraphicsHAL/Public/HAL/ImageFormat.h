#pragma once
#include <FeCore/Strings/Format.h>
#include <FeCore/Strings/String.h>

namespace FE::Graphics::HAL
{
    enum class Format : int32_t
    {
        None,
        R4G4_UNorm_Pack8,
        R4G4B4A4_UNorm_Pack16,
        B4G4R4A4_UNorm_Pack16,
        R5G6B5_UNorm_Pack16,
        B5G6R5_UNorm_Pack16,
        R5G5B5A1_UNorm_Pack16,
        B5G5R5A1_UNorm_Pack16,
        A1R5G5B5_UNorm_Pack16,
        R8_UNorm,
        R8_SNorm,
        R8_UScaled,
        R8_SScaled,
        R8_UInt,
        R8_SInt,
        R8_SRGB,
        R8G8_UNorm,
        R8G8_SNorm,
        R8G8_UScaled,
        R8G8_SScaled,
        R8G8_UInt,
        R8G8_SInt,
        R8G8_SRGB,
        R8G8B8_UNorm,
        R8G8B8_SNorm,
        R8G8B8_UScaled,
        R8G8B8_SScaled,
        R8G8B8_UInt,
        R8G8B8_SInt,
        R8G8B8_SRGB,
        B8G8R8_UNorm,
        B8G8R8_SNorm,
        B8G8R8_UScaled,
        B8G8R8_SScaled,
        B8G8R8_UInt,
        B8G8R8_SInt,
        B8G8R8_SRGB,
        R8G8B8A8_UNorm,
        R8G8B8A8_SNorm,
        R8G8B8A8_UScaled,
        R8G8B8A8_SScaled,
        R8G8B8A8_UInt,
        R8G8B8A8_SInt,
        R8G8B8A8_SRGB,
        B8G8R8A8_UNorm,
        B8G8R8A8_SNorm,
        B8G8R8A8_UScaled,
        B8G8R8A8_SScaled,
        B8G8R8A8_UInt,
        B8G8R8A8_SInt,
        B8G8R8A8_SRGB,
        A8B8G8R8_UNorm_Pack32,
        A8B8G8R8_SNorm_Pack32,
        A8B8G8R8_UScaled_Pack32,
        A8B8G8R8_SScaled_Pack32,
        A8B8G8R8_UInt_Pack32,
        A8B8G8R8_SInt_Pack32,
        A8B8G8R8_SRGB_Pack32,
        A2R10G10B10_UNorm_Pack32,
        A2R10G10B10_SNorm_Pack32,
        A2R10G10B10_UScaled_Pack32,
        A2R10G10B10_SScaled_Pack32,
        A2R10G10B10_UInt_Pack32,
        A2R10G10B10_SInt_Pack32,
        A2B10G10R10_UNorm_Pack32,
        A2B10G10R10_SNorm_Pack32,
        A2B10G10R10_UScaled_Pack32,
        A2B10G10R10_SScaled_Pack32,
        A2B10G10R10_UInt_Pack32,
        A2B10G10R10_SInt_Pack32,
        R16_UNorm,
        R16_SNorm,
        R16_UScaled,
        R16_SScaled,
        R16_UInt,
        R16_SInt,
        R16_SFloat,
        R16G16_UNorm,
        R16G16_SNorm,
        R16G16_UScaled,
        R16G16_SScaled,
        R16G16_UInt,
        R16G16_SInt,
        R16G16_SFloat,
        R16G16B16_UNorm,
        R16G16B16_SNorm,
        R16G16B16_UScaled,
        R16G16B16_SScaled,
        R16G16B16_UInt,
        R16G16B16_SInt,
        R16G16B16_SFloat,
        R16G16B16A16_UNorm,
        R16G16B16A16_SNorm,
        R16G16B16A16_UScaled,
        R16G16B16A16_SScaled,
        R16G16B16A16_UInt,
        R16G16B16A16_SInt,
        R16G16B16A16_SFloat,
        R32_UInt,
        R32_SInt,
        R32_SFloat,
        R32G32_UInt,
        R32G32_SInt,
        R32G32_SFloat,
        R32G32B32_UInt,
        R32G32B32_SInt,
        R32G32B32_SFloat,
        R32G32B32A32_UInt,
        R32G32B32A32_SInt,
        R32G32B32A32_SFloat,
        R64_UInt,
        R64_SInt,
        R64_SFloat,
        R64G64_UInt,
        R64G64_SInt,
        R64G64_SFloat,
        R64G64B64_UInt,
        R64G64B64_SInt,
        R64G64B64_SFloat,
        R64G64B64A64_UInt,
        R64G64B64A64_SInt,
        R64G64B64A64_SFloat,
        B10G11R11_UFloat_Pack32,
        E5B9G9R9_UFloat_Pack32,
        D16_UNorm,
        X8_D24_UNorm_Pack32,
        D32_SFloat,
        S8_UInt,
        D16_UNorm_S8_UInt,
        D24_UNorm_S8_UInt,
        D32_SFloat_S8_UInt,
        BC1_RGB_UNorm_Block,
        BC1_RGB_SRGB_Block,
        BC1_RGBA_UNorm_Block,
        BC1_RGBA_SRGB_Block,
        BC2_UNorm_Block,
        BC2_SRGB_Block,
        BC3_UNorm_Block,
        BC3_SRGB_Block,
        BC4_UNorm_Block,
        BC4_SNorm_Block,
        BC5_UNorm_Block,
        BC5_SNorm_Block,
        BC6H_UFloat_Block,
        BC6H_SFloat_Block,
        BC7_UNorm_Block,
        BC7_SRGB_Block,
        ETC2_R8G8B8_UNorm_Block,
        ETC2_R8G8B8_SRGB_Block,
        ETC2_R8G8B8A1_UNorm_Block,
        ETC2_R8G8B8A1_SRGB_Block,
        ETC2_R8G8B8A8_UNorm_Block,
        ETC2_R8G8B8A8_SRGB_Block,
        EAC_R11_UNorm_Block,
        EAC_R11_SNorm_Block,
        EAC_R11G11_UNorm_Block,
        EAC_R11G11_SNorm_Block,
        ASTC_4x4_UNorm_Block,
        ASTC_4x4_SRGB_Block,
        ASTC_5x4_UNorm_Block,
        ASTC_5x4_SRGB_Block,
        ASTC_5x5_UNorm_Block,
        ASTC_5x5_SRGB_Block,
        ASTC_6x5_UNorm_Block,
        ASTC_6x5_SRGB_Block,
        ASTC_6x6_UNorm_Block,
        ASTC_6x6_SRGB_Block,
        ASTC_8x5_UNorm_Block,
        ASTC_8x5_SRGB_Block,
        ASTC_8x6_UNorm_Block,
        ASTC_8x6_SRGB_Block,
        ASTC_8x8_UNorm_Block,
        ASTC_8x8_SRGB_Block,
        ASTC_10x5_UNorm_Block,
        ASTC_10x5_SRGB_Block,
        ASTC_10x6_UNorm_Block,
        ASTC_10x6_SRGB_Block,
        ASTC_10x8_UNorm_Block,
        ASTC_10x8_SRGB_Block,
        ASTC_10x10_UNorm_Block,
        ASTC_10x10_SRGB_Block,
        ASTC_12x10_UNorm_Block,
        ASTC_12x10_SRGB_Block,
        ASTC_12x12_UNorm_Block,
        ASTC_12x12_SRGB_Block,
        G8B8G8R8_422_UNorm,
        B8G8R8G8_422_UNorm,
        G8_B8_R8_3PLANE_420_UNorm,
        G8_B8R8_2PLANE_420_UNorm,
        G8_B8_R8_3PLANE_422_UNorm,
        G8_B8R8_2PLANE_422_UNorm,
        G8_B8_R8_3PLANE_444_UNorm,
        R10X6_UNorm_Pack16,
        R10X6G10X6_UNorm_2Pack16,
        R10X6G10X6B10X6A10X6_UNorm_4Pack16,
        G10X6B10X6G10X6R10X6_422_UNorm_4Pack16,
        B10X6G10X6R10X6G10X6_422_UNorm_4Pack16,
        G10X6_B10X6_R10X6_3PLANE_420_UNorm_3Pack16,
        G10X6_B10X6R10X6_2PLANE_420_UNorm_3Pack16,
        G10X6_B10X6_R10X6_3PLANE_422_UNorm_3Pack16,
        G10X6_B10X6R10X6_2PLANE_422_UNorm_3Pack16,
        G10X6_B10X6_R10X6_3PLANE_444_UNorm_3Pack16,
        R12X4_UNorm_Pack16,
        R12X4G12X4_UNorm_2Pack16,
        R12X4G12X4B12X4A12X4_UNorm_4Pack16,
        G12X4B12X4G12X4R12X4_422_UNorm_4Pack16,
        B12X4G12X4R12X4G12X4_422_UNorm_4Pack16,
        G12X4_B12X4_R12X4_3PLANE_420_UNorm_3Pack16,
        G12X4_B12X4R12X4_2PLANE_420_UNorm_3Pack16,
        G12X4_B12X4_R12X4_3PLANE_422_UNorm_3Pack16,
        G12X4_B12X4R12X4_2PLANE_422_UNorm_3Pack16,
        G12X4_B12X4_R12X4_3PLANE_444_UNorm_3Pack16,
        G16B16G16R16_422_UNorm,
        B16G16R16G16_422_UNorm,
        G16_B16_R16_3PLANE_420_UNorm,
        G16_B16R16_2PLANE_420_UNorm,
        G16_B16_R16_3PLANE_422_UNorm,
        G16_B16R16_2PLANE_422_UNorm,
        G16_B16_R16_3PLANE_444_UNorm,
        PVRTC1_2BPP_UNorm_Block_IMG,
        PVRTC1_4BPP_UNorm_Block_IMG,
        PVRTC2_2BPP_UNorm_Block_IMG,
        PVRTC2_4BPP_UNorm_Block_IMG,
        PVRTC1_2BPP_SRGB_Block_IMG,
        PVRTC1_4BPP_SRGB_Block_IMG,
        PVRTC2_2BPP_SRGB_Block_IMG,
        PVRTC2_4BPP_SRGB_Block_IMG,
        ASTC_4x4_SFloat_Block_EXT,
        ASTC_5x4_SFloat_Block_EXT,
        ASTC_5x5_SFloat_Block_EXT,
        ASTC_6x5_SFloat_Block_EXT,
        ASTC_6x6_SFloat_Block_EXT,
        ASTC_8x5_SFloat_Block_EXT,
        ASTC_8x6_SFloat_Block_EXT,
        ASTC_8x8_SFloat_Block_EXT,
        ASTC_10x5_SFloat_Block_EXT,
        ASTC_10x6_SFloat_Block_EXT,
        ASTC_10x8_SFloat_Block_EXT,
        ASTC_10x10_SFloat_Block_EXT,
        ASTC_12x10_SFloat_Block_EXT,
        ASTC_12x12_SFloat_Block_EXT,
        G8_B8R8_2PLANE_444_UNorm_EXT,
        G10X6_B10X6R10X6_2PLANE_444_UNorm_3Pack16_EXT,
        G12X4_B12X4R12X4_2PLANE_444_UNorm_3Pack16_EXT,
        G16_B16R16_2PLANE_444_UNorm_EXT,
        A4R4G4B4_UNorm_Pack16_EXT,
        A4B4G4R4_UNorm_Pack16_EXT,
        B10X6G10X6R10X6G10X6_422_UNorm_4Pack16_KHR,
        B12X4G12X4R12X4G12X4_422_UNorm_4Pack16_KHR,
        B16G16R16G16_422_UNorm_KHR,
        B8G8R8G8_422_UNorm_KHR,
        G10X6B10X6G10X6R10X6_422_UNorm_4Pack16_KHR,
        G10X6_B10X6R10X6_2PLANE_420_UNorm_3Pack16_KHR,
        G10X6_B10X6R10X6_2PLANE_422_UNorm_3Pack16_KHR,
        G10X6_B10X6_R10X6_3PLANE_420_UNorm_3Pack16_KHR,
        G10X6_B10X6_R10X6_3PLANE_422_UNorm_3Pack16_KHR,
        G10X6_B10X6_R10X6_3PLANE_444_UNorm_3Pack16_KHR,
        G12X4B12X4G12X4R12X4_422_UNorm_4Pack16_KHR,
        G12X4_B12X4R12X4_2PLANE_420_UNorm_3Pack16_KHR,
        G12X4_B12X4R12X4_2PLANE_422_UNorm_3Pack16_KHR,
        G12X4_B12X4_R12X4_3PLANE_420_UNorm_3Pack16_KHR,
        G12X4_B12X4_R12X4_3PLANE_422_UNorm_3Pack16_KHR,
        G12X4_B12X4_R12X4_3PLANE_444_UNorm_3Pack16_KHR,
        G16B16G16R16_422_UNorm_KHR,
        G16_B16R16_2PLANE_420_UNorm_KHR,
        G16_B16R16_2PLANE_422_UNorm_KHR,
        G16_B16_R16_3PLANE_420_UNorm_KHR,
        G16_B16_R16_3PLANE_422_UNorm_KHR,
        G16_B16_R16_3PLANE_444_UNorm_KHR,
        G8B8G8R8_422_UNorm_KHR,
        G8_B8R8_2PLANE_420_UNorm_KHR,
        G8_B8R8_2PLANE_422_UNorm_KHR,
        G8_B8_R8_3PLANE_420_UNorm_KHR,
        G8_B8_R8_3PLANE_422_UNorm_KHR,
        G8_B8_R8_3PLANE_444_UNorm_KHR,
        R10X6G10X6B10X6A10X6_UNorm_4Pack16_KHR,
        R10X6G10X6_UNorm_2Pack16_KHR,
        R10X6_UNorm_Pack16_KHR,
        R12X4G12X4B12X4A12X4_UNorm_4Pack16_KHR,
        R12X4G12X4_UNorm_2Pack16_KHR,
        R12X4_UNorm_Pack16_KHR,
        Count
    };

    FE_FORCE_INLINE StringSlice ToString(Format format)
    {
        switch (format)
        {
        case Format::None:
            return "None";
        case Format::R4G4_UNorm_Pack8:
            return "R4G4_UNorm_Pack8";
        case Format::R4G4B4A4_UNorm_Pack16:
            return "R4G4B4A4_UNorm_Pack16";
        case Format::B4G4R4A4_UNorm_Pack16:
            return "B4G4R4A4_UNorm_Pack16";
        case Format::R5G6B5_UNorm_Pack16:
            return "R5G6B5_UNorm_Pack16";
        case Format::B5G6R5_UNorm_Pack16:
            return "B5G6R5_UNorm_Pack16";
        case Format::R5G5B5A1_UNorm_Pack16:
            return "R5G5B5A1_UNorm_Pack16";
        case Format::B5G5R5A1_UNorm_Pack16:
            return "B5G5R5A1_UNorm_Pack16";
        case Format::A1R5G5B5_UNorm_Pack16:
            return "A1R5G5B5_UNorm_Pack16";
        case Format::R8_UNorm:
            return "R8_UNorm";
        case Format::R8_SNorm:
            return "R8_SNorm";
        case Format::R8_UScaled:
            return "R8_UScaled";
        case Format::R8_SScaled:
            return "R8_SScaled";
        case Format::R8_UInt:
            return "R8_UInt";
        case Format::R8_SInt:
            return "R8_SInt";
        case Format::R8_SRGB:
            return "R8_SRGB";
        case Format::R8G8_UNorm:
            return "R8G8_UNorm";
        case Format::R8G8_SNorm:
            return "R8G8_SNorm";
        case Format::R8G8_UScaled:
            return "R8G8_UScaled";
        case Format::R8G8_SScaled:
            return "R8G8_SScaled";
        case Format::R8G8_UInt:
            return "R8G8_UInt";
        case Format::R8G8_SInt:
            return "R8G8_SInt";
        case Format::R8G8_SRGB:
            return "R8G8_SRGB";
        case Format::R8G8B8_UNorm:
            return "R8G8B8_UNorm";
        case Format::R8G8B8_SNorm:
            return "R8G8B8_SNorm";
        case Format::R8G8B8_UScaled:
            return "R8G8B8_UScaled";
        case Format::R8G8B8_SScaled:
            return "R8G8B8_SScaled";
        case Format::R8G8B8_UInt:
            return "R8G8B8_UInt";
        case Format::R8G8B8_SInt:
            return "R8G8B8_SInt";
        case Format::R8G8B8_SRGB:
            return "R8G8B8_SRGB";
        case Format::B8G8R8_UNorm:
            return "B8G8R8_UNorm";
        case Format::B8G8R8_SNorm:
            return "B8G8R8_SNorm";
        case Format::B8G8R8_UScaled:
            return "B8G8R8_UScaled";
        case Format::B8G8R8_SScaled:
            return "B8G8R8_SScaled";
        case Format::B8G8R8_UInt:
            return "B8G8R8_UInt";
        case Format::B8G8R8_SInt:
            return "B8G8R8_SInt";
        case Format::B8G8R8_SRGB:
            return "B8G8R8_SRGB";
        case Format::R8G8B8A8_UNorm:
            return "R8G8B8A8_UNorm";
        case Format::R8G8B8A8_SNorm:
            return "R8G8B8A8_SNorm";
        case Format::R8G8B8A8_UScaled:
            return "R8G8B8A8_UScaled";
        case Format::R8G8B8A8_SScaled:
            return "R8G8B8A8_SScaled";
        case Format::R8G8B8A8_UInt:
            return "R8G8B8A8_UInt";
        case Format::R8G8B8A8_SInt:
            return "R8G8B8A8_SInt";
        case Format::R8G8B8A8_SRGB:
            return "R8G8B8A8_SRGB";
        case Format::B8G8R8A8_UNorm:
            return "B8G8R8A8_UNorm";
        case Format::B8G8R8A8_SNorm:
            return "B8G8R8A8_SNorm";
        case Format::B8G8R8A8_UScaled:
            return "B8G8R8A8_UScaled";
        case Format::B8G8R8A8_SScaled:
            return "B8G8R8A8_SScaled";
        case Format::B8G8R8A8_UInt:
            return "B8G8R8A8_UInt";
        case Format::B8G8R8A8_SInt:
            return "B8G8R8A8_SInt";
        case Format::B8G8R8A8_SRGB:
            return "B8G8R8A8_SRGB";
        case Format::A8B8G8R8_UNorm_Pack32:
            return "A8B8G8R8_UNorm_Pack32";
        case Format::A8B8G8R8_SNorm_Pack32:
            return "A8B8G8R8_SNorm_Pack32";
        case Format::A8B8G8R8_UScaled_Pack32:
            return "A8B8G8R8_UScaled_Pack32";
        case Format::A8B8G8R8_SScaled_Pack32:
            return "A8B8G8R8_SScaled_Pack32";
        case Format::A8B8G8R8_UInt_Pack32:
            return "A8B8G8R8_UInt_Pack32";
        case Format::A8B8G8R8_SInt_Pack32:
            return "A8B8G8R8_SInt_Pack32";
        case Format::A8B8G8R8_SRGB_Pack32:
            return "A8B8G8R8_SRGB_Pack32";
        case Format::A2R10G10B10_UNorm_Pack32:
            return "A2R10G10B10_UNorm_Pack32";
        case Format::A2R10G10B10_SNorm_Pack32:
            return "A2R10G10B10_SNorm_Pack32";
        case Format::A2R10G10B10_UScaled_Pack32:
            return "A2R10G10B10_UScaled_Pack32";
        case Format::A2R10G10B10_SScaled_Pack32:
            return "A2R10G10B10_SScaled_Pack32";
        case Format::A2R10G10B10_UInt_Pack32:
            return "A2R10G10B10_UInt_Pack32";
        case Format::A2R10G10B10_SInt_Pack32:
            return "A2R10G10B10_SInt_Pack32";
        case Format::A2B10G10R10_UNorm_Pack32:
            return "A2B10G10R10_UNorm_Pack32";
        case Format::A2B10G10R10_SNorm_Pack32:
            return "A2B10G10R10_SNorm_Pack32";
        case Format::A2B10G10R10_UScaled_Pack32:
            return "A2B10G10R10_UScaled_Pack32";
        case Format::A2B10G10R10_SScaled_Pack32:
            return "A2B10G10R10_SScaled_Pack32";
        case Format::A2B10G10R10_UInt_Pack32:
            return "A2B10G10R10_UInt_Pack32";
        case Format::A2B10G10R10_SInt_Pack32:
            return "A2B10G10R10_SInt_Pack32";
        case Format::R16_UNorm:
            return "R16_UNorm";
        case Format::R16_SNorm:
            return "R16_SNorm";
        case Format::R16_UScaled:
            return "R16_UScaled";
        case Format::R16_SScaled:
            return "R16_SScaled";
        case Format::R16_UInt:
            return "R16_UInt";
        case Format::R16_SInt:
            return "R16_SInt";
        case Format::R16_SFloat:
            return "R16_SFloat";
        case Format::R16G16_UNorm:
            return "R16G16_UNorm";
        case Format::R16G16_SNorm:
            return "R16G16_SNorm";
        case Format::R16G16_UScaled:
            return "R16G16_UScaled";
        case Format::R16G16_SScaled:
            return "R16G16_SScaled";
        case Format::R16G16_UInt:
            return "R16G16_UInt";
        case Format::R16G16_SInt:
            return "R16G16_SInt";
        case Format::R16G16_SFloat:
            return "R16G16_SFloat";
        case Format::R16G16B16_UNorm:
            return "R16G16B16_UNorm";
        case Format::R16G16B16_SNorm:
            return "R16G16B16_SNorm";
        case Format::R16G16B16_UScaled:
            return "R16G16B16_UScaled";
        case Format::R16G16B16_SScaled:
            return "R16G16B16_SScaled";
        case Format::R16G16B16_UInt:
            return "R16G16B16_UInt";
        case Format::R16G16B16_SInt:
            return "R16G16B16_SInt";
        case Format::R16G16B16_SFloat:
            return "R16G16B16_SFloat";
        case Format::R16G16B16A16_UNorm:
            return "R16G16B16A16_UNorm";
        case Format::R16G16B16A16_SNorm:
            return "R16G16B16A16_SNorm";
        case Format::R16G16B16A16_UScaled:
            return "R16G16B16A16_UScaled";
        case Format::R16G16B16A16_SScaled:
            return "R16G16B16A16_SScaled";
        case Format::R16G16B16A16_UInt:
            return "R16G16B16A16_UInt";
        case Format::R16G16B16A16_SInt:
            return "R16G16B16A16_SInt";
        case Format::R16G16B16A16_SFloat:
            return "R16G16B16A16_SFloat";
        case Format::R32_UInt:
            return "R32_UInt";
        case Format::R32_SInt:
            return "R32_SInt";
        case Format::R32_SFloat:
            return "R32_SFloat";
        case Format::R32G32_UInt:
            return "R32G32_UInt";
        case Format::R32G32_SInt:
            return "R32G32_SInt";
        case Format::R32G32_SFloat:
            return "R32G32_SFloat";
        case Format::R32G32B32_UInt:
            return "R32G32B32_UInt";
        case Format::R32G32B32_SInt:
            return "R32G32B32_SInt";
        case Format::R32G32B32_SFloat:
            return "R32G32B32_SFloat";
        case Format::R32G32B32A32_UInt:
            return "R32G32B32A32_UInt";
        case Format::R32G32B32A32_SInt:
            return "R32G32B32A32_SInt";
        case Format::R32G32B32A32_SFloat:
            return "R32G32B32A32_SFloat";
        case Format::R64_UInt:
            return "R64_UInt";
        case Format::R64_SInt:
            return "R64_SInt";
        case Format::R64_SFloat:
            return "R64_SFloat";
        case Format::R64G64_UInt:
            return "R64G64_UInt";
        case Format::R64G64_SInt:
            return "R64G64_SInt";
        case Format::R64G64_SFloat:
            return "R64G64_SFloat";
        case Format::R64G64B64_UInt:
            return "R64G64B64_UInt";
        case Format::R64G64B64_SInt:
            return "R64G64B64_SInt";
        case Format::R64G64B64_SFloat:
            return "R64G64B64_SFloat";
        case Format::R64G64B64A64_UInt:
            return "R64G64B64A64_UInt";
        case Format::R64G64B64A64_SInt:
            return "R64G64B64A64_SInt";
        case Format::R64G64B64A64_SFloat:
            return "R64G64B64A64_SFloat";
        case Format::B10G11R11_UFloat_Pack32:
            return "B10G11R11_UFloat_Pack32";
        case Format::E5B9G9R9_UFloat_Pack32:
            return "E5B9G9R9_UFloat_Pack32";
        case Format::D16_UNorm:
            return "D16_UNorm";
        case Format::X8_D24_UNorm_Pack32:
            return "X8_D24_UNorm_Pack32";
        case Format::D32_SFloat:
            return "D32_SFloat";
        case Format::S8_UInt:
            return "S8_UInt";
        case Format::D16_UNorm_S8_UInt:
            return "D16_UNorm_S8_UInt";
        case Format::D24_UNorm_S8_UInt:
            return "D24_UNorm_S8_UInt";
        case Format::D32_SFloat_S8_UInt:
            return "D32_SFloat_S8_UInt";
        case Format::BC1_RGB_UNorm_Block:
            return "BC1_RGB_UNorm_Block";
        case Format::BC1_RGB_SRGB_Block:
            return "BC1_RGB_SRGB_Block";
        case Format::BC1_RGBA_UNorm_Block:
            return "BC1_RGBA_UNorm_Block";
        case Format::BC1_RGBA_SRGB_Block:
            return "BC1_RGBA_SRGB_Block";
        case Format::BC2_UNorm_Block:
            return "BC2_UNorm_Block";
        case Format::BC2_SRGB_Block:
            return "BC2_SRGB_Block";
        case Format::BC3_UNorm_Block:
            return "BC3_UNorm_Block";
        case Format::BC3_SRGB_Block:
            return "BC3_SRGB_Block";
        case Format::BC4_UNorm_Block:
            return "BC4_UNorm_Block";
        case Format::BC4_SNorm_Block:
            return "BC4_SNorm_Block";
        case Format::BC5_UNorm_Block:
            return "BC5_UNorm_Block";
        case Format::BC5_SNorm_Block:
            return "BC5_SNorm_Block";
        case Format::BC6H_UFloat_Block:
            return "BC6H_UFloat_Block";
        case Format::BC6H_SFloat_Block:
            return "BC6H_SFloat_Block";
        case Format::BC7_UNorm_Block:
            return "BC7_UNorm_Block";
        case Format::BC7_SRGB_Block:
            return "BC7_SRGB_Block";
        case Format::ETC2_R8G8B8_UNorm_Block:
            return "ETC2_R8G8B8_UNorm_Block";
        case Format::ETC2_R8G8B8_SRGB_Block:
            return "ETC2_R8G8B8_SRGB_Block";
        case Format::ETC2_R8G8B8A1_UNorm_Block:
            return "ETC2_R8G8B8A1_UNorm_Block";
        case Format::ETC2_R8G8B8A1_SRGB_Block:
            return "ETC2_R8G8B8A1_SRGB_Block";
        case Format::ETC2_R8G8B8A8_UNorm_Block:
            return "ETC2_R8G8B8A8_UNorm_Block";
        case Format::ETC2_R8G8B8A8_SRGB_Block:
            return "ETC2_R8G8B8A8_SRGB_Block";
        case Format::EAC_R11_UNorm_Block:
            return "EAC_R11_UNorm_Block";
        case Format::EAC_R11_SNorm_Block:
            return "EAC_R11_SNorm_Block";
        case Format::EAC_R11G11_UNorm_Block:
            return "EAC_R11G11_UNorm_Block";
        case Format::EAC_R11G11_SNorm_Block:
            return "EAC_R11G11_SNorm_Block";
        case Format::ASTC_4x4_UNorm_Block:
            return "ASTC_4x4_UNorm_Block";
        case Format::ASTC_4x4_SRGB_Block:
            return "ASTC_4x4_SRGB_Block";
        case Format::ASTC_5x4_UNorm_Block:
            return "ASTC_5x4_UNorm_Block";
        case Format::ASTC_5x4_SRGB_Block:
            return "ASTC_5x4_SRGB_Block";
        case Format::ASTC_5x5_UNorm_Block:
            return "ASTC_5x5_UNorm_Block";
        case Format::ASTC_5x5_SRGB_Block:
            return "ASTC_5x5_SRGB_Block";
        case Format::ASTC_6x5_UNorm_Block:
            return "ASTC_6x5_UNorm_Block";
        case Format::ASTC_6x5_SRGB_Block:
            return "ASTC_6x5_SRGB_Block";
        case Format::ASTC_6x6_UNorm_Block:
            return "ASTC_6x6_UNorm_Block";
        case Format::ASTC_6x6_SRGB_Block:
            return "ASTC_6x6_SRGB_Block";
        case Format::ASTC_8x5_UNorm_Block:
            return "ASTC_8x5_UNorm_Block";
        case Format::ASTC_8x5_SRGB_Block:
            return "ASTC_8x5_SRGB_Block";
        case Format::ASTC_8x6_UNorm_Block:
            return "ASTC_8x6_UNorm_Block";
        case Format::ASTC_8x6_SRGB_Block:
            return "ASTC_8x6_SRGB_Block";
        case Format::ASTC_8x8_UNorm_Block:
            return "ASTC_8x8_UNorm_Block";
        case Format::ASTC_8x8_SRGB_Block:
            return "ASTC_8x8_SRGB_Block";
        case Format::ASTC_10x5_UNorm_Block:
            return "ASTC_10x5_UNorm_Block";
        case Format::ASTC_10x5_SRGB_Block:
            return "ASTC_10x5_SRGB_Block";
        case Format::ASTC_10x6_UNorm_Block:
            return "ASTC_10x6_UNorm_Block";
        case Format::ASTC_10x6_SRGB_Block:
            return "ASTC_10x6_SRGB_Block";
        case Format::ASTC_10x8_UNorm_Block:
            return "ASTC_10x8_UNorm_Block";
        case Format::ASTC_10x8_SRGB_Block:
            return "ASTC_10x8_SRGB_Block";
        case Format::ASTC_10x10_UNorm_Block:
            return "ASTC_10x10_UNorm_Block";
        case Format::ASTC_10x10_SRGB_Block:
            return "ASTC_10x10_SRGB_Block";
        case Format::ASTC_12x10_UNorm_Block:
            return "ASTC_12x10_UNorm_Block";
        case Format::ASTC_12x10_SRGB_Block:
            return "ASTC_12x10_SRGB_Block";
        case Format::ASTC_12x12_UNorm_Block:
            return "ASTC_12x12_UNorm_Block";
        case Format::ASTC_12x12_SRGB_Block:
            return "ASTC_12x12_SRGB_Block";
        case Format::G8B8G8R8_422_UNorm:
            return "G8B8G8R8_422_UNorm";
        case Format::B8G8R8G8_422_UNorm:
            return "B8G8R8G8_422_UNorm";
        case Format::G8_B8_R8_3PLANE_420_UNorm:
            return "G8_B8_R8_3PLANE_420_UNorm";
        case Format::G8_B8R8_2PLANE_420_UNorm:
            return "G8_B8R8_2PLANE_420_UNorm";
        case Format::G8_B8_R8_3PLANE_422_UNorm:
            return "G8_B8_R8_3PLANE_422_UNorm";
        case Format::G8_B8R8_2PLANE_422_UNorm:
            return "G8_B8R8_2PLANE_422_UNorm";
        case Format::G8_B8_R8_3PLANE_444_UNorm:
            return "G8_B8_R8_3PLANE_444_UNorm";
        case Format::R10X6_UNorm_Pack16:
            return "R10X6_UNorm_Pack16";
        case Format::R10X6G10X6_UNorm_2Pack16:
            return "R10X6G10X6_UNorm_2Pack16";
        case Format::R10X6G10X6B10X6A10X6_UNorm_4Pack16:
            return "R10X6G10X6B10X6A10X6_UNorm_4Pack16";
        case Format::G10X6B10X6G10X6R10X6_422_UNorm_4Pack16:
            return "G10X6B10X6G10X6R10X6_422_UNorm_4Pack16";
        case Format::B10X6G10X6R10X6G10X6_422_UNorm_4Pack16:
            return "B10X6G10X6R10X6G10X6_422_UNorm_4Pack16";
        case Format::G10X6_B10X6_R10X6_3PLANE_420_UNorm_3Pack16:
            return "G10X6_B10X6_R10X6_3PLANE_420_UNorm_3Pack16";
        case Format::G10X6_B10X6R10X6_2PLANE_420_UNorm_3Pack16:
            return "G10X6_B10X6R10X6_2PLANE_420_UNorm_3Pack16";
        case Format::G10X6_B10X6_R10X6_3PLANE_422_UNorm_3Pack16:
            return "G10X6_B10X6_R10X6_3PLANE_422_UNorm_3Pack16";
        case Format::G10X6_B10X6R10X6_2PLANE_422_UNorm_3Pack16:
            return "G10X6_B10X6R10X6_2PLANE_422_UNorm_3Pack16";
        case Format::G10X6_B10X6_R10X6_3PLANE_444_UNorm_3Pack16:
            return "G10X6_B10X6_R10X6_3PLANE_444_UNorm_3Pack16";
        case Format::R12X4_UNorm_Pack16:
            return "R12X4_UNorm_Pack16";
        case Format::R12X4G12X4_UNorm_2Pack16:
            return "R12X4G12X4_UNorm_2Pack16";
        case Format::R12X4G12X4B12X4A12X4_UNorm_4Pack16:
            return "R12X4G12X4B12X4A12X4_UNorm_4Pack16";
        case Format::G12X4B12X4G12X4R12X4_422_UNorm_4Pack16:
            return "G12X4B12X4G12X4R12X4_422_UNorm_4Pack16";
        case Format::B12X4G12X4R12X4G12X4_422_UNorm_4Pack16:
            return "B12X4G12X4R12X4G12X4_422_UNorm_4Pack16";
        case Format::G12X4_B12X4_R12X4_3PLANE_420_UNorm_3Pack16:
            return "G12X4_B12X4_R12X4_3PLANE_420_UNorm_3Pack16";
        case Format::G12X4_B12X4R12X4_2PLANE_420_UNorm_3Pack16:
            return "G12X4_B12X4R12X4_2PLANE_420_UNorm_3Pack16";
        case Format::G12X4_B12X4_R12X4_3PLANE_422_UNorm_3Pack16:
            return "G12X4_B12X4_R12X4_3PLANE_422_UNorm_3Pack16";
        case Format::G12X4_B12X4R12X4_2PLANE_422_UNorm_3Pack16:
            return "G12X4_B12X4R12X4_2PLANE_422_UNorm_3Pack16";
        case Format::G12X4_B12X4_R12X4_3PLANE_444_UNorm_3Pack16:
            return "G12X4_B12X4_R12X4_3PLANE_444_UNorm_3Pack16";
        case Format::G16B16G16R16_422_UNorm:
            return "G16B16G16R16_422_UNorm";
        case Format::B16G16R16G16_422_UNorm:
            return "B16G16R16G16_422_UNorm";
        case Format::G16_B16_R16_3PLANE_420_UNorm:
            return "G16_B16_R16_3PLANE_420_UNorm";
        case Format::G16_B16R16_2PLANE_420_UNorm:
            return "G16_B16R16_2PLANE_420_UNorm";
        case Format::G16_B16_R16_3PLANE_422_UNorm:
            return "G16_B16_R16_3PLANE_422_UNorm";
        case Format::G16_B16R16_2PLANE_422_UNorm:
            return "G16_B16R16_2PLANE_422_UNorm";
        case Format::G16_B16_R16_3PLANE_444_UNorm:
            return "G16_B16_R16_3PLANE_444_UNorm";
        case Format::PVRTC1_2BPP_UNorm_Block_IMG:
            return "PVRTC1_2BPP_UNorm_Block_IMG";
        case Format::PVRTC1_4BPP_UNorm_Block_IMG:
            return "PVRTC1_4BPP_UNorm_Block_IMG";
        case Format::PVRTC2_2BPP_UNorm_Block_IMG:
            return "PVRTC2_2BPP_UNorm_Block_IMG";
        case Format::PVRTC2_4BPP_UNorm_Block_IMG:
            return "PVRTC2_4BPP_UNorm_Block_IMG";
        case Format::PVRTC1_2BPP_SRGB_Block_IMG:
            return "PVRTC1_2BPP_SRGB_Block_IMG";
        case Format::PVRTC1_4BPP_SRGB_Block_IMG:
            return "PVRTC1_4BPP_SRGB_Block_IMG";
        case Format::PVRTC2_2BPP_SRGB_Block_IMG:
            return "PVRTC2_2BPP_SRGB_Block_IMG";
        case Format::PVRTC2_4BPP_SRGB_Block_IMG:
            return "PVRTC2_4BPP_SRGB_Block_IMG";
        case Format::ASTC_4x4_SFloat_Block_EXT:
            return "ASTC_4x4_SFloat_Block_EXT";
        case Format::ASTC_5x4_SFloat_Block_EXT:
            return "ASTC_5x4_SFloat_Block_EXT";
        case Format::ASTC_5x5_SFloat_Block_EXT:
            return "ASTC_5x5_SFloat_Block_EXT";
        case Format::ASTC_6x5_SFloat_Block_EXT:
            return "ASTC_6x5_SFloat_Block_EXT";
        case Format::ASTC_6x6_SFloat_Block_EXT:
            return "ASTC_6x6_SFloat_Block_EXT";
        case Format::ASTC_8x5_SFloat_Block_EXT:
            return "ASTC_8x5_SFloat_Block_EXT";
        case Format::ASTC_8x6_SFloat_Block_EXT:
            return "ASTC_8x6_SFloat_Block_EXT";
        case Format::ASTC_8x8_SFloat_Block_EXT:
            return "ASTC_8x8_SFloat_Block_EXT";
        case Format::ASTC_10x5_SFloat_Block_EXT:
            return "ASTC_10x5_SFloat_Block_EXT";
        case Format::ASTC_10x6_SFloat_Block_EXT:
            return "ASTC_10x6_SFloat_Block_EXT";
        case Format::ASTC_10x8_SFloat_Block_EXT:
            return "ASTC_10x8_SFloat_Block_EXT";
        case Format::ASTC_10x10_SFloat_Block_EXT:
            return "ASTC_10x10_SFloat_Block_EXT";
        case Format::ASTC_12x10_SFloat_Block_EXT:
            return "ASTC_12x10_SFloat_Block_EXT";
        case Format::ASTC_12x12_SFloat_Block_EXT:
            return "ASTC_12x12_SFloat_Block_EXT";
        case Format::G8_B8R8_2PLANE_444_UNorm_EXT:
            return "G8_B8R8_2PLANE_444_UNorm_EXT";
        case Format::G10X6_B10X6R10X6_2PLANE_444_UNorm_3Pack16_EXT:
            return "G10X6_B10X6R10X6_2PLANE_444_UNorm_3Pack16_EXT";
        case Format::G12X4_B12X4R12X4_2PLANE_444_UNorm_3Pack16_EXT:
            return "G12X4_B12X4R12X4_2PLANE_444_UNorm_3Pack16_EXT";
        case Format::G16_B16R16_2PLANE_444_UNorm_EXT:
            return "G16_B16R16_2PLANE_444_UNorm_EXT";
        case Format::A4R4G4B4_UNorm_Pack16_EXT:
            return "A4R4G4B4_UNorm_Pack16_EXT";
        case Format::A4B4G4R4_UNorm_Pack16_EXT:
            return "A4B4G4R4_UNorm_Pack16_EXT";
        case Format::B10X6G10X6R10X6G10X6_422_UNorm_4Pack16_KHR:
            return "B10X6G10X6R10X6G10X6_422_UNorm_4Pack16_KHR";
        case Format::B12X4G12X4R12X4G12X4_422_UNorm_4Pack16_KHR:
            return "B12X4G12X4R12X4G12X4_422_UNorm_4Pack16_KHR";
        case Format::B16G16R16G16_422_UNorm_KHR:
            return "B16G16R16G16_422_UNorm_KHR";
        case Format::B8G8R8G8_422_UNorm_KHR:
            return "B8G8R8G8_422_UNorm_KHR";
        case Format::G10X6B10X6G10X6R10X6_422_UNorm_4Pack16_KHR:
            return "G10X6B10X6G10X6R10X6_422_UNorm_4Pack16_KHR";
        case Format::G10X6_B10X6R10X6_2PLANE_420_UNorm_3Pack16_KHR:
            return "G10X6_B10X6R10X6_2PLANE_420_UNorm_3Pack16_KHR";
        case Format::G10X6_B10X6R10X6_2PLANE_422_UNorm_3Pack16_KHR:
            return "G10X6_B10X6R10X6_2PLANE_422_UNorm_3Pack16_KHR";
        case Format::G10X6_B10X6_R10X6_3PLANE_420_UNorm_3Pack16_KHR:
            return "G10X6_B10X6_R10X6_3PLANE_420_UNorm_3Pack16_KHR";
        case Format::G10X6_B10X6_R10X6_3PLANE_422_UNorm_3Pack16_KHR:
            return "G10X6_B10X6_R10X6_3PLANE_422_UNorm_3Pack16_KHR";
        case Format::G10X6_B10X6_R10X6_3PLANE_444_UNorm_3Pack16_KHR:
            return "G10X6_B10X6_R10X6_3PLANE_444_UNorm_3Pack16_KHR";
        case Format::G12X4B12X4G12X4R12X4_422_UNorm_4Pack16_KHR:
            return "G12X4B12X4G12X4R12X4_422_UNorm_4Pack16_KHR";
        case Format::G12X4_B12X4R12X4_2PLANE_420_UNorm_3Pack16_KHR:
            return "G12X4_B12X4R12X4_2PLANE_420_UNorm_3Pack16_KHR";
        case Format::G12X4_B12X4R12X4_2PLANE_422_UNorm_3Pack16_KHR:
            return "G12X4_B12X4R12X4_2PLANE_422_UNorm_3Pack16_KHR";
        case Format::G12X4_B12X4_R12X4_3PLANE_420_UNorm_3Pack16_KHR:
            return "G12X4_B12X4_R12X4_3PLANE_420_UNorm_3Pack16_KHR";
        case Format::G12X4_B12X4_R12X4_3PLANE_422_UNorm_3Pack16_KHR:
            return "G12X4_B12X4_R12X4_3PLANE_422_UNorm_3Pack16_KHR";
        case Format::G12X4_B12X4_R12X4_3PLANE_444_UNorm_3Pack16_KHR:
            return "G12X4_B12X4_R12X4_3PLANE_444_UNorm_3Pack16_KHR";
        case Format::G16B16G16R16_422_UNorm_KHR:
            return "G16B16G16R16_422_UNorm_KHR";
        case Format::G16_B16R16_2PLANE_420_UNorm_KHR:
            return "G16_B16R16_2PLANE_420_UNorm_KHR";
        case Format::G16_B16R16_2PLANE_422_UNorm_KHR:
            return "G16_B16R16_2PLANE_422_UNorm_KHR";
        case Format::G16_B16_R16_3PLANE_420_UNorm_KHR:
            return "G16_B16_R16_3PLANE_420_UNorm_KHR";
        case Format::G16_B16_R16_3PLANE_422_UNorm_KHR:
            return "G16_B16_R16_3PLANE_422_UNorm_KHR";
        case Format::G16_B16_R16_3PLANE_444_UNorm_KHR:
            return "G16_B16_R16_3PLANE_444_UNorm_KHR";
        case Format::G8B8G8R8_422_UNorm_KHR:
            return "G8B8G8R8_422_UNorm_KHR";
        case Format::G8_B8R8_2PLANE_420_UNorm_KHR:
            return "G8_B8R8_2PLANE_420_UNorm_KHR";
        case Format::G8_B8R8_2PLANE_422_UNorm_KHR:
            return "G8_B8R8_2PLANE_422_UNorm_KHR";
        case Format::G8_B8_R8_3PLANE_420_UNorm_KHR:
            return "G8_B8_R8_3PLANE_420_UNorm_KHR";
        case Format::G8_B8_R8_3PLANE_422_UNorm_KHR:
            return "G8_B8_R8_3PLANE_422_UNorm_KHR";
        case Format::G8_B8_R8_3PLANE_444_UNorm_KHR:
            return "G8_B8_R8_3PLANE_444_UNorm_KHR";
        case Format::R10X6G10X6B10X6A10X6_UNorm_4Pack16_KHR:
            return "R10X6G10X6B10X6A10X6_UNorm_4Pack16_KHR";
        case Format::R10X6G10X6_UNorm_2Pack16_KHR:
            return "R10X6G10X6_UNorm_2Pack16_KHR";
        case Format::R10X6_UNorm_Pack16_KHR:
            return "R10X6_UNorm_Pack16_KHR";
        case Format::R12X4G12X4B12X4A12X4_UNorm_4Pack16_KHR:
            return "R12X4G12X4B12X4A12X4_UNorm_4Pack16_KHR";
        case Format::R12X4G12X4_UNorm_2Pack16_KHR:
            return "R12X4G12X4_UNorm_2Pack16_KHR";
        case Format::R12X4_UNorm_Pack16_KHR:
            return "R12X4_UNorm_Pack16_KHR";
        default:
            return "{Error Format}";
        }
    }

    FE_FORCE_INLINE uint32_t GetFormatSize(Format format)
    {
        switch (format)
        {
        case Format::None:
            return 0;
        case Format::R4G4_UNorm_Pack8:
            return 1;
        case Format::R4G4B4A4_UNorm_Pack16:
        case Format::B4G4R4A4_UNorm_Pack16:
        case Format::R5G6B5_UNorm_Pack16:
        case Format::B5G6R5_UNorm_Pack16:
        case Format::R5G5B5A1_UNorm_Pack16:
        case Format::B5G5R5A1_UNorm_Pack16:
        case Format::A1R5G5B5_UNorm_Pack16:
            return 2;
        case Format::R8_UNorm:
        case Format::R8_SNorm:
        case Format::R8_UScaled:
        case Format::R8_SScaled:
        case Format::R8_UInt:
        case Format::R8_SInt:
        case Format::R8_SRGB:
            return 1;
        case Format::R8G8_UNorm:
        case Format::R8G8_SNorm:
        case Format::R8G8_UScaled:
        case Format::R8G8_SScaled:
        case Format::R8G8_UInt:
        case Format::R8G8_SInt:
        case Format::R8G8_SRGB:
            return 2;
        case Format::R8G8B8_UNorm:
        case Format::R8G8B8_SNorm:
        case Format::R8G8B8_UScaled:
        case Format::R8G8B8_SScaled:
        case Format::R8G8B8_UInt:
        case Format::R8G8B8_SInt:
        case Format::R8G8B8_SRGB:
        case Format::B8G8R8_UNorm:
        case Format::B8G8R8_SNorm:
        case Format::B8G8R8_UScaled:
        case Format::B8G8R8_SScaled:
        case Format::B8G8R8_UInt:
        case Format::B8G8R8_SInt:
        case Format::B8G8R8_SRGB:
            return 3;
        case Format::R8G8B8A8_UNorm:
        case Format::R8G8B8A8_SNorm:
        case Format::R8G8B8A8_UScaled:
        case Format::R8G8B8A8_SScaled:
        case Format::R8G8B8A8_UInt:
        case Format::R8G8B8A8_SInt:
        case Format::R8G8B8A8_SRGB:
        case Format::B8G8R8A8_UNorm:
        case Format::B8G8R8A8_SNorm:
        case Format::B8G8R8A8_UScaled:
        case Format::B8G8R8A8_SScaled:
        case Format::B8G8R8A8_UInt:
        case Format::B8G8R8A8_SInt:
        case Format::B8G8R8A8_SRGB:
        case Format::A8B8G8R8_UNorm_Pack32:
        case Format::A8B8G8R8_SNorm_Pack32:
        case Format::A8B8G8R8_UScaled_Pack32:
        case Format::A8B8G8R8_SScaled_Pack32:
        case Format::A8B8G8R8_UInt_Pack32:
        case Format::A8B8G8R8_SInt_Pack32:
        case Format::A8B8G8R8_SRGB_Pack32:
        case Format::A2R10G10B10_UNorm_Pack32:
        case Format::A2R10G10B10_SNorm_Pack32:
        case Format::A2R10G10B10_UScaled_Pack32:
        case Format::A2R10G10B10_SScaled_Pack32:
        case Format::A2R10G10B10_UInt_Pack32:
        case Format::A2R10G10B10_SInt_Pack32:
        case Format::A2B10G10R10_UNorm_Pack32:
        case Format::A2B10G10R10_SNorm_Pack32:
        case Format::A2B10G10R10_UScaled_Pack32:
        case Format::A2B10G10R10_SScaled_Pack32:
        case Format::A2B10G10R10_UInt_Pack32:
        case Format::A2B10G10R10_SInt_Pack32:
            return 4;
        case Format::R16_UNorm:
        case Format::R16_SNorm:
        case Format::R16_UScaled:
        case Format::R16_SScaled:
        case Format::R16_UInt:
        case Format::R16_SInt:
        case Format::R16_SFloat:
            return 2;
        case Format::R16G16_UNorm:
        case Format::R16G16_SNorm:
        case Format::R16G16_UScaled:
        case Format::R16G16_SScaled:
        case Format::R16G16_UInt:
        case Format::R16G16_SInt:
        case Format::R16G16_SFloat:
            return 4;
        case Format::R16G16B16_UNorm:
        case Format::R16G16B16_SNorm:
        case Format::R16G16B16_UScaled:
        case Format::R16G16B16_SScaled:
        case Format::R16G16B16_UInt:
        case Format::R16G16B16_SInt:
        case Format::R16G16B16_SFloat:
            return 6;
        case Format::R16G16B16A16_UNorm:
        case Format::R16G16B16A16_SNorm:
        case Format::R16G16B16A16_UScaled:
        case Format::R16G16B16A16_SScaled:
        case Format::R16G16B16A16_UInt:
        case Format::R16G16B16A16_SInt:
        case Format::R16G16B16A16_SFloat:
            return 8;
        case Format::R32_UInt:
        case Format::R32_SInt:
        case Format::R32_SFloat:
            return 4;
        case Format::R32G32_UInt:
        case Format::R32G32_SInt:
        case Format::R32G32_SFloat:
            return 8;
        case Format::R32G32B32_UInt:
        case Format::R32G32B32_SInt:
        case Format::R32G32B32_SFloat:
            return 12;
        case Format::R32G32B32A32_UInt:
        case Format::R32G32B32A32_SInt:
        case Format::R32G32B32A32_SFloat:
            return 16;
        case Format::R64_UInt:
        case Format::R64_SInt:
        case Format::R64_SFloat:
            return 8;
        case Format::R64G64_UInt:
        case Format::R64G64_SInt:
        case Format::R64G64_SFloat:
            return 16;
        case Format::R64G64B64_UInt:
        case Format::R64G64B64_SInt:
        case Format::R64G64B64_SFloat:
            return 24;
        case Format::R64G64B64A64_UInt:
        case Format::R64G64B64A64_SInt:
        case Format::R64G64B64A64_SFloat:
            return 32;
        case Format::B10G11R11_UFloat_Pack32:
        case Format::E5B9G9R9_UFloat_Pack32:
            return 4;
        case Format::D16_UNorm:
            return 2;
        case Format::X8_D24_UNorm_Pack32:
        case Format::D32_SFloat:
            return 4;
        case Format::S8_UInt:
            return 1;
        case Format::D16_UNorm_S8_UInt:
            return 3;
        case Format::D24_UNorm_S8_UInt:
            return 4;
        case Format::D32_SFloat_S8_UInt:
        case Format::BC1_RGB_UNorm_Block:
        case Format::BC1_RGB_SRGB_Block:
        case Format::BC1_RGBA_UNorm_Block:
        case Format::BC1_RGBA_SRGB_Block:
            return 8;
        case Format::BC2_UNorm_Block:
        case Format::BC2_SRGB_Block:
        case Format::BC3_UNorm_Block:
        case Format::BC3_SRGB_Block:
            return 16;
        case Format::BC4_UNorm_Block:
        case Format::BC4_SNorm_Block:
            return 8;
        case Format::BC5_UNorm_Block:
        case Format::BC5_SNorm_Block:
        case Format::BC6H_UFloat_Block:
        case Format::BC6H_SFloat_Block:
        case Format::BC7_UNorm_Block:
        case Format::BC7_SRGB_Block:
            return 16;
        case Format::ETC2_R8G8B8_UNorm_Block:
        case Format::ETC2_R8G8B8_SRGB_Block:
        case Format::ETC2_R8G8B8A1_UNorm_Block:
        case Format::ETC2_R8G8B8A1_SRGB_Block:
            return 8;
        case Format::ETC2_R8G8B8A8_UNorm_Block:
        case Format::ETC2_R8G8B8A8_SRGB_Block:
            return 16;
        case Format::EAC_R11_UNorm_Block:
        case Format::EAC_R11_SNorm_Block:
            return 8;
        case Format::EAC_R11G11_UNorm_Block:
        case Format::EAC_R11G11_SNorm_Block:
        case Format::ASTC_4x4_UNorm_Block:
        case Format::ASTC_4x4_SRGB_Block:
        case Format::ASTC_5x4_UNorm_Block:
        case Format::ASTC_5x4_SRGB_Block:
        case Format::ASTC_5x5_UNorm_Block:
        case Format::ASTC_5x5_SRGB_Block:
        case Format::ASTC_6x5_UNorm_Block:
        case Format::ASTC_6x5_SRGB_Block:
        case Format::ASTC_6x6_UNorm_Block:
        case Format::ASTC_6x6_SRGB_Block:
        case Format::ASTC_8x5_UNorm_Block:
        case Format::ASTC_8x5_SRGB_Block:
        case Format::ASTC_8x6_UNorm_Block:
        case Format::ASTC_8x6_SRGB_Block:
        case Format::ASTC_8x8_UNorm_Block:
        case Format::ASTC_8x8_SRGB_Block:
        case Format::ASTC_10x5_UNorm_Block:
        case Format::ASTC_10x5_SRGB_Block:
        case Format::ASTC_10x6_UNorm_Block:
        case Format::ASTC_10x6_SRGB_Block:
        case Format::ASTC_10x8_UNorm_Block:
        case Format::ASTC_10x8_SRGB_Block:
        case Format::ASTC_10x10_UNorm_Block:
        case Format::ASTC_10x10_SRGB_Block:
        case Format::ASTC_12x10_UNorm_Block:
        case Format::ASTC_12x10_SRGB_Block:
        case Format::ASTC_12x12_UNorm_Block:
        case Format::ASTC_12x12_SRGB_Block:
            return 16;
        case Format::PVRTC1_2BPP_UNorm_Block_IMG:
        case Format::PVRTC1_4BPP_UNorm_Block_IMG:
        case Format::PVRTC2_2BPP_UNorm_Block_IMG:
        case Format::PVRTC2_4BPP_UNorm_Block_IMG:
        case Format::PVRTC1_2BPP_SRGB_Block_IMG:
        case Format::PVRTC1_4BPP_SRGB_Block_IMG:
        case Format::PVRTC2_2BPP_SRGB_Block_IMG:
        case Format::PVRTC2_4BPP_SRGB_Block_IMG:
            return 8;
        case Format::R10X6_UNorm_Pack16:
            return 2;
        case Format::R10X6G10X6_UNorm_2Pack16:
            return 4;
        case Format::R10X6G10X6B10X6A10X6_UNorm_4Pack16:
            return 8;
        case Format::R12X4_UNorm_Pack16:
            return 2;
        case Format::R12X4G12X4_UNorm_2Pack16:
            return 4;
        case Format::R12X4G12X4B12X4A12X4_UNorm_4Pack16:
            return 8;
        case Format::G8B8G8R8_422_UNorm:
        case Format::B8G8R8G8_422_UNorm:
            return 4;
        case Format::G10X6B10X6G10X6R10X6_422_UNorm_4Pack16:
        case Format::B10X6G10X6R10X6G10X6_422_UNorm_4Pack16:
        case Format::G12X4B12X4G12X4R12X4_422_UNorm_4Pack16:
        case Format::B12X4G12X4R12X4G12X4_422_UNorm_4Pack16:
        case Format::G16B16G16R16_422_UNorm:
        case Format::B16G16R16G16_422_UNorm:
            return 8;
        case Format::G8_B8_R8_3PLANE_420_UNorm:
        case Format::G8_B8R8_2PLANE_420_UNorm:
            return 6;
        case Format::G10X6_B10X6_R10X6_3PLANE_420_UNorm_3Pack16:
        case Format::G10X6_B10X6R10X6_2PLANE_420_UNorm_3Pack16:
        case Format::G12X4_B12X4_R12X4_3PLANE_420_UNorm_3Pack16:
        case Format::G12X4_B12X4R12X4_2PLANE_420_UNorm_3Pack16:
        case Format::G16_B16_R16_3PLANE_420_UNorm:
        case Format::G16_B16R16_2PLANE_420_UNorm:
            return 12;
        case Format::G8_B8_R8_3PLANE_422_UNorm:
        case Format::G8_B8R8_2PLANE_422_UNorm:
            return 4;
        case Format::G10X6_B10X6_R10X6_3PLANE_422_UNorm_3Pack16:
        case Format::G10X6_B10X6R10X6_2PLANE_422_UNorm_3Pack16:
        case Format::G12X4_B12X4_R12X4_3PLANE_422_UNorm_3Pack16:
        case Format::G12X4_B12X4R12X4_2PLANE_422_UNorm_3Pack16:
        case Format::G16_B16_R16_3PLANE_422_UNorm:
        case Format::G16_B16R16_2PLANE_422_UNorm:
            return 8;
        case Format::G8_B8_R8_3PLANE_444_UNorm:
            return 3;
        case Format::G10X6_B10X6_R10X6_3PLANE_444_UNorm_3Pack16:
        case Format::G12X4_B12X4_R12X4_3PLANE_444_UNorm_3Pack16:
        case Format::G16_B16_R16_3PLANE_444_UNorm:
            return 6;
        default:
            return 0;
        }
    }
} // namespace FE::Graphics::HAL

namespace FE::Fmt
{
    template<>
    struct ValueFormatter<FE::Graphics::HAL::Format> : BasicValueFormatter<FE::Graphics::HAL::Format>
    {
        void Format(String& buffer, const FE::Graphics::HAL::Format& value) const override
        {
            buffer.Append(FE::Graphics::HAL::ToString(value));
        }
    };
} // namespace FE::Fmt
