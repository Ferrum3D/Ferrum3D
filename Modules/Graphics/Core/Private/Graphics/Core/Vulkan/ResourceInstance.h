#pragma once
#include <Graphics/Core/Common/ResourceInstance.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Vulkan
{
    struct ResourceInstance : public Common::ResourceInstance
    {
        FE_RTTI("7748916A-053B-447D-82D1-C417599323D7");

        VmaAllocation m_vmaAllocation = nullptr;
    };


    struct BufferInstance final : public ResourceInstance
    {
        FE_RTTI("82BD426F-A6C0-45BD-9F67-8223CA9B70CC");

        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkBufferView m_view = VK_NULL_HANDLE;

        static BufferInstance* Create();
        static void Delete(BufferInstance* instance);

        void Invalidate(VkDevice device);
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

        static TextureInstance* Create();
        static void Delete(TextureInstance* instance);

        void Invalidate(VkDevice device);
    };
} // namespace FE::Graphics::Vulkan
