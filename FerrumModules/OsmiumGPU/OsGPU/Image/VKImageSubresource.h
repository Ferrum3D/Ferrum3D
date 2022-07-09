#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Image/ImageSubresource.h>

namespace FE::Osmium
{
    inline VkImageAspectFlags VKConvert(ImageAspect aspect)
    {
        switch(aspect)
        {
        case ImageAspect::Color:
            return VK_IMAGE_ASPECT_COLOR_BIT;
        case ImageAspect::Depth:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case ImageAspect::Stencil:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        default:
            FE_UNREACHABLE("Unknown ImageAspect");
            return VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
        }
    }

    inline VkImageAspectFlags VKConvert(ImageAspectFlags aspect)
    {
        VkImageAspectFlags result{};
        if ((aspect & ImageAspectFlags::Color) != ImageAspectFlags::None)
            result |= VK_IMAGE_ASPECT_COLOR_BIT;
        if ((aspect & ImageAspectFlags::Depth) != ImageAspectFlags::None)
            result |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if ((aspect & ImageAspectFlags::Stencil) != ImageAspectFlags::None)
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
        result.layerCount     = range.ArraySliceCount;
        result.baseMipLevel   = range.MinMipSlice;
        result.levelCount     = range.MipSliceCount;
        result.aspectMask     = VKConvert(range.AspectFlags);
        return result;
    }
} // namespace FE::Osmium
