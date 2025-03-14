#pragma once
#include <Graphics/Core/ImageBase.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <festd/vector.h>

namespace FE::Graphics::Vulkan
{
    struct Image final : public Core::Image
    {
        FE_RTTI_Class(Image, "9726C432-92C1-489C-9623-55330B3530E8");

        ~Image() override;

        static Image* Create(Core::Device* device);

        void InitInternal(Env::Name name, const Core::ImageDesc& desc, VkImage nativeImage);

        void InitInternal(VmaAllocator allocator, Env::Name name, const Core::ImageDesc& desc);

        [[nodiscard]] VkImage GetNative() const
        {
            return m_nativeImage;
        }

        [[nodiscard]] VkImageView GetWholeResourceView() const
        {
            return m_view;
        }

        [[nodiscard]] VkImageView GetSubresourceView(const Core::ImageSubresource& subresource);

        const Core::ImageDesc& GetDesc() const override;

    private:
        explicit Image(Core::Device* device);

        void InitView();

        VkImage m_nativeImage = VK_NULL_HANDLE;
        VmaAllocation m_vmaAllocation = VK_NULL_HANDLE;
        VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;

        struct ViewCacheEntry final
        {
            uint32_t m_key;
            VkImageView m_view = VK_NULL_HANDLE;
        };

        Core::ImageSubresource m_wholeImageSubresource;
        festd::small_vector<ViewCacheEntry, 6> m_viewCache;

        Core::ImageDesc m_desc;
    };

    FE_ENABLE_NATIVE_CAST(Image);
} // namespace FE::Graphics::Vulkan
