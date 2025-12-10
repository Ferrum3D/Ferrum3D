#pragma once
#include <Graphics/Core/ImageFormat.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    inline VkFormat Translate(const Core::Format format)
    {
        const VkFormat VK_FORMAT_BC1_UNORM_BLOCK = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        const VkFormat VK_FORMAT_BC1_SRGB_BLOCK = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

        switch (format)
        {
        case Core::Format::kUndefined:
            return VK_FORMAT_UNDEFINED;

#define FE_DECL_VK_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                     \
    case Core::Format::k##name:                                                                                                  \
        return VK_FORMAT_##name;

#define FE_DECL_VK_BC_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                  \
    case Core::Format::k##name:                                                                                                  \
        return VK_FORMAT_##name##_BLOCK;

#define FE_DECL_VK_PACK_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                \
    case Core::Format::k##name:                                                                                                  \
        return VK_FORMAT_##name##_PACK32;

            FE_EXPAND_BASIC_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_DS_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_BC_FORMATS(FE_DECL_VK_BC_FORMAT_CONVERSION)
            FE_EXPAND_PACK_FORMATS(FE_DECL_VK_PACK_FORMAT_CONVERSION)

#undef FE_DECL_VK_FORMAT_CONVERSION
#undef FE_DECL_VK_BC_FORMAT_CONVERSION
#undef FE_DECL_VK_PACK_FORMAT_CONVERSION

        default:
            FE_DebugBreak();
            return VK_FORMAT_UNDEFINED;
        }
    }


    inline Core::Format Translate(const VkFormat format)
    {
        const VkFormat VK_FORMAT_BC1_UNORM_BLOCK = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        const VkFormat VK_FORMAT_BC1_SRGB_BLOCK = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

        switch (format)
        {
        case VK_FORMAT_UNDEFINED:
            return Core::Format::kUndefined;

#define FE_DECL_VK_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                     \
    case VK_FORMAT_##name:                                                                                                       \
        return Core::Format::k##name;

#define FE_DECL_VK_BC_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                  \
    case VK_FORMAT_##name##_BLOCK:                                                                                               \
        return Core::Format::k##name;

#define FE_DECL_VK_PACK_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                \
    case VK_FORMAT_##name##_PACK32:                                                                                              \
        return Core::Format::k##name;

            FE_EXPAND_BASIC_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_DS_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_BC_FORMATS(FE_DECL_VK_BC_FORMAT_CONVERSION)
            FE_EXPAND_PACK_FORMATS(FE_DECL_VK_PACK_FORMAT_CONVERSION)

#undef FE_DECL_VK_FORMAT_CONVERSION
#undef FE_DECL_VK_BC_FORMAT_CONVERSION
#undef FE_DECL_VK_PACK_FORMAT_CONVERSION

        default:
            FE_DebugBreak();
            return Core::Format::kUndefined;
        }
    }


    inline bool operator==(const VkFormat lhs, const Core::Format rhs)
    {
        return lhs == Translate(rhs);
    }


    inline bool operator!=(const VkFormat lhs, const Core::Format rhs)
    {
        return !(lhs == rhs);
    }


    inline bool operator==(const Core::Format lhs, const VkFormat rhs)
    {
        return Translate(lhs) == rhs;
    }


    inline bool operator!=(const Core::Format lhs, const VkFormat rhs)
    {
        return !(lhs == rhs);
    }


    inline VkImageAspectFlags TranslateImageAspectFlags(const Core::ImageAspect aspect)
    {
        switch (aspect)
        {
        default:
            FE_DebugBreak();
            [[fallthrough]];

        case Core::ImageAspect::kColor:
            return VK_IMAGE_ASPECT_COLOR_BIT;
        case Core::ImageAspect::kDepth:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case Core::ImageAspect::kStencil:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        case Core::ImageAspect::kDepthStencil:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }


    inline VkImageAspectFlags TranslateImageAspectFlags(const Core::Format format)
    {
        const Core::FormatInfo formatInfo{ format };
        return TranslateImageAspectFlags(formatInfo.m_aspectFlags);
    }
} // namespace FE::Graphics::Vulkan
