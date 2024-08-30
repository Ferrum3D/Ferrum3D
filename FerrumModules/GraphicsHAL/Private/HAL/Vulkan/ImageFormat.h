#pragma once
#include <HAL/ImageFormat.h>
#include <HAL/Vulkan/Common/Config.h>
#include <array>
#include <tuple>

namespace FE::Graphics::Vulkan
{
    inline VkFormat VKConvert(HAL::Format format)
    {
        const VkFormat VK_FORMAT_BC1_UNORM_BLOCK = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        const VkFormat VK_FORMAT_BC1_SRGB_BLOCK = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

        switch (format)
        {
#define FE_DECL_VK_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                     \
    case HAL::Format::k##name:                                                                                                   \
        return VK_FORMAT_##name;

#define FE_DECL_VK_BC_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                  \
    case HAL::Format::k##name:                                                                                                   \
        return VK_FORMAT_##name##_BLOCK;

            FE_EXPAND_BASIC_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_DS_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_BC_FORMATS(FE_DECL_VK_BC_FORMAT_CONVERSION)

#undef FE_DECL_VK_FORMAT_CONVERSION
#undef FE_DECL_VK_BC_FORMAT_CONVERSION

        case HAL::Format::kR11G11B10_SFLOAT:
            return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

        default:
            FE_DebugBreak();
            return VK_FORMAT_UNDEFINED;
        }
    }

    inline HAL::Format VKConvert(VkFormat format)
    {
        const VkFormat VK_FORMAT_BC1_UNORM_BLOCK = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        const VkFormat VK_FORMAT_BC1_SRGB_BLOCK = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;

        switch (format)
        {
#define FE_DECL_VK_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                     \
    case VK_FORMAT_##name:                                                                                                       \
        return HAL::Format::k##name;

#define FE_DECL_VK_BC_FORMAT_CONVERSION(name, type, byteSize, channelCount, aspectFlags, bc, sign, srgb, index)                  \
    case VK_FORMAT_##name##_BLOCK:                                                                                               \
        return HAL::Format::k##name;

            FE_EXPAND_BASIC_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_DS_FORMATS(FE_DECL_VK_FORMAT_CONVERSION)
            FE_EXPAND_BC_FORMATS(FE_DECL_VK_BC_FORMAT_CONVERSION)

#undef FE_DECL_VK_FORMAT_CONVERSION
#undef FE_DECL_VK_BC_FORMAT_CONVERSION

        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            return HAL::Format::kR11G11B10_SFLOAT;

        default:
            FE_DebugBreak();
            return HAL::Format::kUndefined;
        }
    }

    inline bool operator==(VkFormat lhs, HAL::Format rhs)
    {
        return lhs == VKConvert(rhs);
    }

    inline bool operator!=(VkFormat lhs, HAL::Format rhs)
    {
        return !(lhs == rhs);
    }

    inline bool operator==(HAL::Format lhs, VkFormat rhs)
    {
        return VKConvert(lhs) == rhs;
    }

    inline bool operator!=(HAL::Format lhs, VkFormat rhs)
    {
        return !(lhs == rhs);
    }
} // namespace FE::Graphics::Vulkan
