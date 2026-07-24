#include <Core/Memory/PoolAllocator.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/ResourceInstance.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        Memory::SpinLockedPoolAllocator GBufferInstancePool{ "Graphics/BufferInstancePool", sizeof(BufferInstance) };
        Memory::SpinLockedPoolAllocator GTextureInstancePool{ "Graphics/TextureInstancePool", sizeof(TextureInstance) };


        VkBufferUsageFlags TranslateBufferUsage(const Core::BarrierAccessFlags accessFlags, const bool isTexelBuffer)
        {
            FE_Assert((accessFlags & Core::BarrierAccessFlags::kAllBufferAccessMask) == accessFlags);

            VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            if (Bit::AnySet(accessFlags, Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite))
            {
                usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
                usage |= isTexelBuffer ? VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kIndexBuffer))
                usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kVertexBuffer))
                usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kConstantBuffer))
                usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kIndirectArgument))
                usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kAccelerationStructureRead))
                usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kAccelerationStructureWrite))
                usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;

            return usage;
        }
    } // namespace


    void TranslateBufferDesc(const Core::BufferDesc bufferDesc, const Core::ResourceCommitParams params,
                             VkBufferCreateInfo& bufferCI, VmaAllocationCreateInfo& allocationCI)
    {
        const bool isTexelBuffer = bufferDesc.m_format != Core::Format::kUndefined;

        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = bufferDesc.m_size;
        bufferCI.usage = TranslateBufferUsage(params.m_bindFlags, isTexelBuffer);

        allocationCI.usage = VMA_MEMORY_USAGE_AUTO;
        switch (params.m_memory)
        {
        default:
        case Core::ResourceMemory::kNotCommitted:
            FE_DebugBreak();
            [[fallthrough]];

        case Core::ResourceMemory::kDeviceLocal:
            allocationCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;

        case Core::ResourceMemory::kHostRandomAccess:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            break;

        case Core::ResourceMemory::kHostWriteThrough:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            break;
        }
    }


    BufferInstance* BufferInstance::Create(const Core::BufferDesc desc, Core::ResourceCommitParams commitParams,
                                           Core::ResourcePool* pool)
    {
        auto* instance = Memory::New<BufferInstance>(&GBufferInstancePool);
        instance->m_type = Core::ResourceType::kBuffer;
        instance->m_bufferDesc = desc;
        instance->m_bindFlags = commitParams.m_bindFlags;
        instance->m_memoryStatus = commitParams.m_memory;
        instance->m_pool = pool;
        return instance;
    }


    void BufferInstance::Delete(BufferInstance* instance)
    {
        Memory::Delete(&GBufferInstancePool, instance);
    }


    void BufferInstance::Invalidate(const Core::Device* device)
    {
        if (m_buffer)
        {
            const VmaAllocator allocator = ImplCast(device)->GetVmaInstance();
            vmaDestroyBuffer(allocator, m_buffer, m_vmaAllocation);

            if (m_view)
                vkDestroyBufferView(NativeCast(device), m_view, nullptr);

            m_vmaAllocation = nullptr;
            m_buffer = VK_NULL_HANDLE;
            m_view = VK_NULL_HANDLE;
        }
        else
        {
            FE_Assert(m_vmaAllocation == nullptr);
        }
    }


    TextureInstance* TextureInstance::Create(const Core::TextureDesc desc, const Core::ResourceCommitParams commitParams,
                                             Core::ResourcePool* pool)
    {
        auto* instance = Memory::New<TextureInstance>(&GTextureInstancePool);
        instance->m_type = Core::ResourceType::kTexture;
        instance->m_textureDesc = desc;
        instance->m_bindFlags = commitParams.m_bindFlags;
        instance->m_memoryStatus = commitParams.m_memory;
        instance->m_pool = pool;
        return instance;
    }


    void TextureInstance::Delete(TextureInstance* instance)
    {
        Memory::Delete(&GTextureInstancePool, instance);
    }


    void TextureInstance::Invalidate(const Core::Device* device)
    {
        FE_PROFILER_ZONE();

        if (m_image)
        {
            const VkDevice vkDevice = NativeCast(device);

            FE_Assert(!m_subresourceStates.empty());
            m_subresourceStates.clear();

            for (const auto& [subresource, view] : m_viewCache)
                vkDestroyImageView(vkDevice, view, nullptr);

            m_viewCache.clear();

            FE_Assert(m_wholeImageView);
            vkDestroyImageView(vkDevice, m_wholeImageView, nullptr);

            if (m_vmaAllocation)
            {
                const VmaAllocator allocator = ImplCast(device)->GetVmaInstance();
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
