#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Format.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>

namespace FE::Graphics::Vulkan
{
    FE_DECLARE_VULKAN_OBJECT_POOL(Buffer);


    Buffer* Buffer::Create(Core::Device* device, const Env::Name name, const Core::BufferDesc desc)
    {
        FE_PROFILER_ZONE();
        return new (GBufferPool.AllocateMemory()) Buffer(device, name, desc);
    }


    Buffer::Buffer(Core::Device* device, const Env::Name name, const Core::BufferDesc desc)
    {
        m_device = device;
        m_name = name;
        m_desc = desc;
        m_type = Core::ResourceType::kBuffer;
        Register();
    }


    void Buffer::DestroyObject()
    {
        GBufferPool.Delete(this);
    }


    void Buffer::UpdateDebugNames() const
    {
        if (m_instance == nullptr)
            return;

        auto* bufferInstance = Rtti::AssertCast<BufferInstance*>(m_instance);

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(bufferInstance->m_buffer);
        nameInfo.pObjectName = m_name.c_str();
        VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));

        if (bufferInstance->m_view)
        {
            nameInfo.objectType = VK_OBJECT_TYPE_BUFFER_VIEW;
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(bufferInstance->m_view);
            nameInfo.pObjectName = m_name.c_str();
            VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));
        }

        if (bufferInstance->m_vmaAllocation)
        {
            vmaSetAllocationName(ImplCast(m_device)->GetVmaInstance(), bufferInstance->m_vmaAllocation, m_name.c_str());
        }
    }


    void* Buffer::Map()
    {
        FE_PROFILER_ZONE();

        FE_Assert(m_instance);

        const auto* bufferInstance = Rtti::AssertCast<BufferInstance*>(m_instance);
        FE_Assert(bufferInstance->m_memoryStatus == Core::ResourceMemory::kHostRandomAccess
                  || bufferInstance->m_memoryStatus == Core::ResourceMemory::kHostWriteThrough);

        const VmaAllocator allocator = ImplCast(m_device)->GetVmaInstance();
        const VmaAllocation allocation = bufferInstance->m_vmaAllocation;

        void* result;
        VerifyVk(vmaMapMemory(allocator, allocation, &result));
        return result;
    }


    void Buffer::Unmap()
    {
        const auto* bufferInstance = Rtti::AssertCast<BufferInstance*>(m_instance);
        const VmaAllocator allocator = ImplCast(m_device)->GetVmaInstance();
        const VmaAllocation allocation = bufferInstance->m_vmaAllocation;
        vmaUnmapMemory(allocator, allocation);
    }


    void Buffer::FlushMappedRange(const uint32_t offset, const uint32_t byteSize)
    {
        FE_Assert(m_instance);

        const auto* bufferInstance = Rtti::AssertCast<BufferInstance*>(m_instance);
        FE_Assert(bufferInstance->m_memoryStatus == Core::ResourceMemory::kHostRandomAccess
                  || bufferInstance->m_memoryStatus == Core::ResourceMemory::kHostWriteThrough);

        const VmaAllocator allocator = ImplCast(m_device)->GetVmaInstance();
        const VmaAllocation allocation = bufferInstance->m_vmaAllocation;
        VerifyVk(vmaFlushAllocation(allocator, allocation, offset, byteSize));
    }


    void Buffer::DecommitMemory()
    {
        if (m_instance == nullptr)
            return;

        FE_Assert(m_instance->m_pool, "Externally created buffers not implemented");
        m_instance->m_pool->DecommitBufferMemory(this);
    }


    void Buffer::CommitInternal(ResourcePool* resourcePool, const Core::ResourceCommitParams params)
    {
        FE_PROFILER_ZONE();

        FE_Assert(m_instance == nullptr);

        m_instance = BufferInstance::Create(m_desc, params, resourcePool);
        m_instance->m_subresourceStates.push_back(Common::SubresourceState{});

        const bool isTexelBuffer = m_desc.m_format != Core::Format::kUndefined;

        VkBufferCreateInfo bufferCI = {};
        VmaAllocationCreateInfo allocationCI = {};
        TranslateBufferDesc(m_desc, params, bufferCI, allocationCI);

        const VmaAllocator allocator = ImplCast(m_device)->GetVmaInstance();
        auto* bufferInstance = Rtti::AssertCast<BufferInstance*>(m_instance);
        VerifyVk(vmaCreateBuffer(allocator,
                                 &bufferCI,
                                 &allocationCI,
                                 &bufferInstance->m_buffer,
                                 &bufferInstance->m_vmaAllocation,
                                 nullptr));

        if (isTexelBuffer)
        {
            VkBufferViewCreateInfo viewCI{};
            viewCI.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            viewCI.buffer = bufferInstance->m_buffer;
            viewCI.format = Translate(m_desc.m_format);
            viewCI.offset = 0;
            viewCI.range = m_desc.m_size;
            VerifyVk(vkCreateBufferView(NativeCast(m_device), &viewCI, nullptr, &bufferInstance->m_view));
        }

        UpdateDebugNames();
    }


    void Buffer::SwapInternal(BufferInstance*& instance)
    {
        if (instance != nullptr)
        {
            FE_Assert(m_desc == instance->m_bufferDesc);
            FE_Assert(instance->m_memoryStatus != Core::ResourceMemory::kNotCommitted);
        }

        BufferInstance* oldInstance = static_cast<BufferInstance*>(m_instance);
        m_instance = instance;
        instance = oldInstance;

        UpdateDebugNames();
    }


    Buffer::~Buffer()
    {
        DecommitMemory();
    }
} // namespace FE::Graphics::Vulkan
