#pragma once
#include <HAL/ImageSubresource.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::HAL
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
        result.arrayLayer = subresource.ArraySlice;
        result.aspectMask = VKConvert(subresource.Aspect);
        result.mipLevel = subresource.MipSlice;
        return result;
    }

    inline VkImageSubresourceRange VKConvert(const ImageSubresourceRange& range)
    {
        VkImageSubresourceRange result{};
        result.baseArrayLayer = range.MinArraySlice;
        result.layerCount = range.ArraySliceCount;
        result.baseMipLevel = range.MinMipSlice;
        result.levelCount = range.MipSliceCount;
        result.aspectMask = VKConvert(range.AspectFlags);
        return result;
    }
} // namespace FE::Graphics::HAL
