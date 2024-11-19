#pragma once
#include <Graphics/RHI/ImageFormat.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>
#include <array>
#include <tuple>

namespace FE::Graphics::Vulkan
{
    inline VkFormat VKConvert(RHI::Format format)
    {
        const VkFormat VK_FORMAT_BC1_UNORM_BLOCK = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        const VkFormat VK_FORMAT_BC1_SRGB_BLOCK = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

        switch (format)
        {
#define FE_DECL_VK_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                     \
    case RHI::Format::k##name:                                                                                                   \
        return VK_FORMAT_##name;

#define FE_DECL_VK_BC_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                  \
    case RHI::Format::k##name:                                                                                                   \
        return VK_FORMAT_##name##_BLOCK;

            FE_EXPAND_BASIC_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_DS_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_BC_FORMATS(FE_DECL_VK_BC_FORMAT_CONVERSION)

#undef FE_DECL_VK_FORMAT_CONVERSION
#undef FE_DECL_VK_BC_FORMAT_CONVERSION

        case RHI::Format::kR11G11B10_SFLOAT:
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

        default:
            FE_DebugBreak();
            return VK_FORMAT_UNDEFINED;
        }
    }


    inline RHI::Format VKConvert(VkFormat format)
    {
        const VkFormat VK_FORMAT_BC1_UNORM_BLOCK = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        const VkFormat VK_FORMAT_BC1_SRGB_BLOCK = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

        switch (format)
        {
#define FE_DECL_VK_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                     \
    case VK_FORMAT_##name:                                                                                                       \
        return RHI::Format::k##name;

#define FE_DECL_VK_BC_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                  \
    case VK_FORMAT_##name##_BLOCK:                                                                                               \
        return RHI::Format::k##name;

            FE_EXPAND_BASIC_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_DS_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_BC_FORMATS(FE_DECL_VK_BC_FORMAT_CONVERSION)

#undef FE_DECL_VK_FORMAT_CONVERSION
#undef FE_DECL_VK_BC_FORMAT_CONVERSION

        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return RHI::Format::kR11G11B10_SFLOAT;

        default:
            FE_DebugBreak();
            return RHI::Format::kUndefined;
        }
    }


    inline bool operator==(VkFormat lhs, RHI::Format rhs)
    {
        return lhs == VKConvert(rhs);
    }


    inline bool operator!=(VkFormat lhs, RHI::Format rhs)
    {
        return !(lhs == rhs);
    }


    inline bool operator==(RHI::Format lhs, VkFormat rhs)
    {
        return VKConvert(lhs) == rhs;
    }


    inline bool operator!=(RHI::Format lhs, VkFormat rhs)
    {
        return !(lhs == rhs);
    }
} // namespace FE::Graphics::Vulkan
