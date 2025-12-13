#pragma once
#include <Graphics/Core/Barrier.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/Format.h>

namespace FE::Graphics::Vulkan
{
    struct Device;

    inline VkImageSubresourceRange TranslateSubresourceRange(const Core::TextureSubresource subresource)
    {
        VkImageSubresourceRange vkSubresource = {};
        vkSubresource.baseMipLevel = subresource.m_mostDetailedMipSlice;
        vkSubresource.levelCount = subresource.m_mipSliceCount;
        vkSubresource.baseArrayLayer = subresource.m_firstArraySlice;
        vkSubresource.layerCount = subresource.m_arraySize;
        vkSubresource.aspectMask = TranslateImageAspectFlags(subresource.m_aspect);
        return vkSubresource;
    }


    VkPipelineStageFlags2 TranslateSyncFlags(Core::BarrierSyncFlags flags);
    VkAccessFlags2 TranslateAccessFlags(Core::BarrierAccessFlags flags);
    VkImageLayout TranslateImageLayout(Core::BarrierLayout layout);

    VkImageMemoryBarrier2 TranslateBarrier(const Core::TextureBarrierDesc& barrier, const Device* device);
    VkBufferMemoryBarrier2 TranslateBarrier(const Core::BufferBarrierDesc& barrier, const Device* device);
} // namespace FE::Graphics::Vulkan
