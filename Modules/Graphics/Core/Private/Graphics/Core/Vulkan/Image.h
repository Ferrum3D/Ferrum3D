#pragma once
#include <Graphics/Core/ImageBase.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <festd/vector.h>

namespace FE::Graphics::Vulkan
{
    inline VkImageAspectFlags TranslateImageAspectFlags(const Core::ImageAspect aspect)
    {
        switch (aspect)
        {
        default:
            FE_DebugBreak();
            [[fallthrough]];

        case Core::ImageAspect::kColor:
            return VK_IMAGE_ASPECT_COLOR_BIT;
        case Core::ImageAspect::kDepth:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case Core::ImageAspect::kStencil:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
        case Core::ImageAspect::kDepthStencil:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }


    inline VkImageAspectFlags TranslateImageAspectFlags(const Core::Format format)
    {
        const Core::FormatInfo formatInfo{ format };
        return TranslateImageAspectFlags(formatInfo.m_aspectFlags);
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

        [[nodiscard]] VkImageView GetSubresourceView(VkDevice device, const Core::ImageSubresource& subresource) const;

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
        mutable festd::inline_vector<ViewCacheEntry, 6> m_viewCache;

        Core::ImageDesc m_desc = {};
    };
} // namespace FE::Graphics::Vulkan
