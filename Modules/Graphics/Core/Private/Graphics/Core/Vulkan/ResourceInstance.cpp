#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Core/Vulkan/ResourceInstance.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        Memory::SpinLockedPoolAllocator GBufferInstancePool{ "BufferInstancePool", sizeof(BufferInstance) };
        Memory::SpinLockedPoolAllocator GTextureInstancePool{ "TextureInstancePool", sizeof(TextureInstance) };
    } // namespace


    BufferInstance* BufferInstance::Create()
    {
        return Memory::New<BufferInstance>(&GBufferInstancePool);
    }


    void BufferInstance::Delete(BufferInstance* instance)
    {
        Memory::Delete(&GBufferInstancePool, instance);
    }


    void BufferInstance::Invalidate(const VkDevice device)
    {
        if (m_buffer)
        {
            const VmaAllocator allocator = ImplCast(m_pool)->GetAllocator();
            vmaDestroyBuffer(allocator, m_buffer, m_vmaAllocation);

            if (m_view)
                vkDestroyBufferView(device, m_view, nullptr);

            m_vmaAllocation = nullptr;
            m_buffer = VK_NULL_HANDLE;
            m_view = VK_NULL_HANDLE;
        }
        else
        {
            FE_Assert(m_vmaAllocation == nullptr);
        }
    }


    TextureInstance* TextureInstance::Create()
    {
        return Memory::New<TextureInstance>(&GTextureInstancePool);
    }


    void TextureInstance::Delete(TextureInstance* instance)
    {
        Memory::Delete(&GTextureInstancePool, instance);
    }


    void TextureInstance::Invalidate(const VkDevice device)
    {
        FE_PROFILER_ZONE();

        if (m_image)
        {
            FE_Assert(!m_subresourceStates.empty());
            m_subresourceStates.clear();

            for (const auto& [subresource, view] : m_viewCache)
                vkDestroyImageView(device, view, nullptr);

            m_viewCache.clear();

            FE_Assert(m_wholeImageView);
            vkDestroyImageView(device, m_wholeImageView, nullptr);

            if (m_vmaAllocation)
            {
                const VmaAllocator allocator = ImplCast(m_pool)->GetAllocator();
                vmaDestroyImage(allocator, m_image, m_vmaAllocation);
            }

            m_image = VK_NULL_HANDLE;
            m_memoryStatus = Core::ResourceMemory::kNotCommitted;
        }
        else
        {
            FE_Assert(m_memoryStatus == Core::ResourceMemory::kNotCommitted);
            FE_Assert(m_subresourceStates.empty());
            FE_Assert(m_viewCache.empty());
            FE_Assert(m_wholeImageView == VK_NULL_HANDLE);
            FE_Assert(m_vmaAllocation == nullptr);
        }
    }
} // namespace FE::Graphics::Vulkan
