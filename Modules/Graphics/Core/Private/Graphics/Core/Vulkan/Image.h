#pragma once
#include <Graphics/Core/ImageBase.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <festd/vector.h>

namespace FE::Graphics::Vulkan
{
    inline VkImageAspectFlags TranslateImageAspectFlags(const Core::Format format)
    {
        const Core::FormatInfo formatInfo{ format };

        VkImageAspectFlags aspectMask = 0;
        if (Bit::AllSet(formatInfo.m_aspectFlags, Core::ImageAspectFlags::kColor))
            aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
        if (Bit::AllSet(formatInfo.m_aspectFlags, Core::ImageAspectFlags::kDepth))
            aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if (Bit::AllSet(formatInfo.m_aspectFlags, Core::ImageAspectFlags::kStencil))
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

        return aspectMask;
    }


    struct Device;

    struct Image
    {
        void Shutdown(VkDevice device);

        void InitInternal(VkDevice device, const Core::ImageDesc& desc, VkImage nativeImage);

        [[nodiscard]] VkImage GetNative() const
        {
            return m_nativeImage;
        }

        [[nodiscard]] VkImageView GetWholeResourceView() const
        {
            return m_view;
        }

        [[nodiscard]] VkImageView GetSubresourceView(VkDevice device, const Core::ImageSubresource& subresource);

    protected:
        void InitInternal(VkDevice device, const char* name, VmaAllocator allocator, VkImageUsageFlags usage,
                          const Core::ImageDesc& desc);

        void InitView(VkDevice device);

        VkImage m_nativeImage = VK_NULL_HANDLE;
        VmaAllocation m_vmaAllocation = VK_NULL_HANDLE;
        VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;

        struct ViewCacheEntry final
        {
            uint32_t m_key;
            VkImageView m_view = VK_NULL_HANDLE;
        };

        Core::ImageSubresource m_wholeImageSubresource = Core::ImageSubresource::kInvalid;
        festd::small_vector<ViewCacheEntry, 6> m_viewCache;

        Core::ImageDesc m_desc = {};
    };
} // namespace FE::Graphics::Vulkan
