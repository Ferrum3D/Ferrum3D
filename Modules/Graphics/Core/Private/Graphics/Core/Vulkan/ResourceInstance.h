#pragma once
#include <Graphics/Core/Common/ResourceInstance.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    struct ResourcePool;

    void TranslateBufferDesc(Core::BufferDesc bufferDesc, Core::ResourceCommitParams params, VkBufferCreateInfo& bufferCI,
                             VmaAllocationCreateInfo& allocationCI);


    struct BufferInstance final : public Common::ResourceInstance
    {
        FE_RTTI("82BD426F-A6C0-45BD-9F67-8223CA9B70CC");

        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkBufferView m_view = VK_NULL_HANDLE;
        VmaAllocation m_vmaAllocation = nullptr;

        static BufferInstance* Create(Core::BufferDesc desc, Core::ResourceCommitParams commitParams,
                                      Core::ResourcePool* pool = nullptr);
        static void Delete(BufferInstance* instance);

        void Allocate(const Core::Device* device);
        void Invalidate(const Core::Device* device);

        void UpdateDebugNames(Core::Device* device, Env::Name name) override;

        void* Map(Core::Device* device) override;
        void Unmap(Core::Device* device) override;
        void FlushMappedRange(Core::Device* device, uint32_t offset, uint32_t byteSize) override;
    };


    struct TextureInstance final : public Common::ResourceInstance
    {
        FE_RTTI("C47D99B8-920D-49B6-A1B9-0F24143C3FEB");

        struct ViewCacheEntry final
        {
            Core::TextureSubresource m_subresource;
            VkImageView m_view;
        };

        VkImage m_image = VK_NULL_HANDLE;
        VmaAllocation m_vmaAllocation = nullptr;

        VkImageView m_wholeImageView = VK_NULL_HANDLE;

        Threading::SpinLock m_viewCacheLock;
        festd::inline_vector<ViewCacheEntry, 4> m_viewCache;

        static TextureInstance* Create(Core::TextureDesc desc, Core::ResourceCommitParams commitParams,
                                       Core::ResourcePool* pool = nullptr);
        static void Delete(TextureInstance* instance);

        VkImageView GetSubresourceView(const Core::Device* device, Core::TextureSubresource subresource);

        void Allocate(const Core::Device* device);
        void Invalidate(const Core::Device* device);

        void UpdateDebugNames(Core::Device* device, Env::Name name) override;

        void* Map(Core::Device* device) override;
        void Unmap(Core::Device* device) override;
        void FlushMappedRange(Core::Device* device, uint32_t offset, uint32_t byteSize) override;

    private:
        void InitWholeImageView(const Core::Device* device);
    };


    BufferInstance* GetInstance(const Core::Buffer* buffer);
    VkBuffer NativeCast(const Core::Buffer* buffer);

    TextureInstance* GetInstance(const Core::Texture* texture);
    VkImage NativeCast(const Core::Texture* texture);
} // namespace FE::Graphics::Vulkan
