#pragma once
#include <Graphics/Core/ImageFormat.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <array>
#include <tuple>

namespace FE::Graphics::Vulkan
{
    inline VkFormat Translate(const Core::Format format)
    {
        const VkFormat VK_FORMAT_BC1_UNORM_BLOCK = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        const VkFormat VK_FORMAT_BC1_SRGB_BLOCK = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

        switch (format)
        {
#define FE_DECL_VK_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                     \
    case Core::Format::k##name:                                                                                                   \
        return VK_FORMAT_##name;

#define FE_DECL_VK_BC_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                  \
    case Core::Format::k##name:                                                                                                   \
        return VK_FORMAT_##name##_BLOCK;

            FE_EXPAND_BASIC_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_DS_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_BC_FORMATS(FE_DECL_VK_BC_FORMAT_CONVERSION)

#undef FE_DECL_VK_FORMAT_CONVERSION
#undef FE_DECL_VK_BC_FORMAT_CONVERSION

        case Core::Format::kR11G11B10_SFLOAT:
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

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
#define FE_DECL_VK_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                     \
    case VK_FORMAT_##name:                                                                                                       \
        return Core::Format::k##name;

#define FE_DECL_VK_BC_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                  \
    case VK_FORMAT_##name##_BLOCK:                                                                                               \
        return Core::Format::k##name;

            FE_EXPAND_BASIC_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_DS_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_BC_FORMATS(FE_DECL_VK_BC_FORMAT_CONVERSION)

#undef FE_DECL_VK_FORMAT_CONVERSION
#undef FE_DECL_VK_BC_FORMAT_CONVERSION

        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return Core::Format::kR11G11B10_SFLOAT;

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
} // namespace FE::Graphics::Vulkan
