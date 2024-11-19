#pragma once
#include <Graphics/RHI/ImageSubresource.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::RHI
{
    inline VkImageAspectFlags VKConvert(ImageAspect aspect)
    {
        switch (aspect)
        {
        case ImageAspect::kColor:
            return VK_IMAGE_ASPECT_COLOR_BIT;
        case ImageAspect::kDepth:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case ImageAspect::kStencil:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        default:
            FE_AssertMsg(false, "Unknown ImageAspect");
            return VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
        }
    }


    inline VkImageAspectFlags VKConvert(ImageAspectFlags aspect)
    {
        VkImageAspectFlags result{};
        if ((aspect & ImageAspectFlags::kColor) != ImageAspectFlags::kNone)
            result |= VK_IMAGE_ASPECT_COLOR_BIT;
        if ((aspect & ImageAspectFlags::kDepth) != ImageAspectFlags::kNone)
            result |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if ((aspect & ImageAspectFlags::kStencil) != ImageAspectFlags::kNone)
            result |= VK_IMAGE_ASPECT_STENCIL_BIT;
        return result;
    }


    inline VkImageSubresource VKConvert(const ImageSubresource& subresource)
    {
        VkImageSubresource result{};
        result.arrayLayer = subresource.m_arraySlice;
        result.aspectMask = VKConvert(subresource.m_aspect);
        result.mipLevel = subresource.m_mipSlice;
        return result;
    }


    inline VkImageSubresourceRange VKConvert(const ImageSubresourceRange& range)
    {
        VkImageSubresourceRange result{};
        result.baseArrayLayer = range.m_minArraySlice;
        result.layerCount = range.m_arraySliceCount;
        result.baseMipLevel = range.m_minMipSlice;
        result.levelCount = range.m_mipSliceCount;
        result.aspectMask = VKConvert(range.m_aspectFlags);
        return result;
    }
} // namespace FE::Graphics::RHI
