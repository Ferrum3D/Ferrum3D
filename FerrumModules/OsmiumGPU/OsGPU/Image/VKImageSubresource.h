#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Image/ImageSubresource.h>

namespace FE::Osmium
{
    inline vk::ImageAspectFlags VKConvert(ImageAspect aspect)
    {
        switch(aspect)
        {
        case ImageAspect::RenderTarget:
            return vk::ImageAspectFlagBits::eColor;
        case ImageAspect::Depth:
            return vk::ImageAspectFlagBits::eDepth;
        case ImageAspect::Stencil:
            return vk::ImageAspectFlagBits::eStencil;
        default:
            FE_UNREACHABLE("Unknown ImageAspect");
            return static_cast<vk::ImageAspectFlagBits>(-1);
        }
    }

    inline vk::ImageAspectFlags VKConvert(ImageAspectFlags aspect)
    {
        vk::ImageAspectFlags result{};
        if ((aspect & ImageAspectFlags::RenderTarget) != ImageAspectFlags::None)
            result |= vk::ImageAspectFlagBits::eColor;
        if ((aspect & ImageAspectFlags::Depth) != ImageAspectFlags::None)
            result |= vk::ImageAspectFlagBits::eDepth;
        if ((aspect & ImageAspectFlags::Stencil) != ImageAspectFlags::None)
            result |= vk::ImageAspectFlagBits::eStencil;
        return result;
    }

    inline vk::ImageSubresource VKConvert(const ImageSubresource& subresource)
    {
        vk::ImageSubresource result{};
        result.arrayLayer = subresource.ArraySlice;
        result.aspectMask = VKConvert(subresource.Aspect);
        result.mipLevel = subresource.MipSlice;
        return result;
    }

    inline vk::ImageSubresourceRange VKConvert(const ImageSubresourceRange& range)
    {
        vk::ImageSubresourceRange result{};
        result.baseArrayLayer = range.MinArraySlice;
        result.layerCount     = range.ArraySliceCount;
        result.baseMipLevel   = range.MinMipSlice;
        result.levelCount     = range.MipSliceCount;
        result.aspectMask     = VKConvert(range.AspectFlags);
        return result;
    }
} // namespace FE::Osmium
