#pragma once
#include <Graphics/Core/Common/ResourceInstance.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Vulkan
{
    void TranslateBufferDesc(Core::BufferDesc bufferDesc, Core::ResourceCommitParams params, VkBufferCreateInfo& bufferCI,
                             VmaAllocationCreateInfo& allocationCI);


    struct ResourceInstance : public Common::ResourceInstance
    {
        FE_RTTI("7748916A-053B-447D-82D1-C417599323D7");

        festd::array<uint64_t, festd::to_underlying(Core::DeviceQueueType::kCount)> m_lastFenceValues = {};
        VmaAllocation m_vmaAllocation = nullptr;
    };


    struct BufferInstance final : public ResourceInstance
    {
        FE_RTTI("82BD426F-A6C0-45BD-9F67-8223CA9B70CC");

        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkBufferView m_view = VK_NULL_HANDLE;

        static BufferInstance* Create(Core::BufferDesc desc, Core::ResourceCommitParams commitParams,
                                      Core::ResourcePool* pool = nullptr);
        static void Delete(BufferInstance* instance);

        void Invalidate(const Core::Device* device);
    };


    struct TextureInstance final : public ResourceInstance
    {
        FE_RTTI("C47D99B8-920D-49B6-A1B9-0F24143C3FEB");

        struct ViewCacheEntry final
        {
            Core::TextureSubresource m_subresource;
            VkImageView m_view;
        };

        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_wholeImageView = VK_NULL_HANDLE;
        festd::inline_vector<ViewCacheEntry, 4> m_viewCache;

        static TextureInstance* Create(Core::TextureDesc desc, Core::ResourceCommitParams commitParams,
                                       Core::ResourcePool* pool = nullptr);
        static void Delete(TextureInstance* instance);

        void Invalidate(const Core::Device* device);
    };
} // namespace FE::Graphics::Vulkan
