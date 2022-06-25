#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Image/ImageSubresource.h>

namespace FE::Osmium
{
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
