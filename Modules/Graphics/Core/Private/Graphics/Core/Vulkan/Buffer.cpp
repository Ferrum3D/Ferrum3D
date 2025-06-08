#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/Device.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        VulkanObjectPoolType GBufferPool{ "VulkanBufferPool", sizeof(Buffer) };
    }


    Buffer* Buffer::Create(Core::Device* device)
    {
        FE_PROFILER_ZONE();

        return Rc<Buffer>::Allocate(&GBufferPool, [device](void* memory) {
            return new (memory) Buffer(device);
        });
    }


    Buffer::Buffer(Core::Device* device)
    {
        m_device = device;
        m_type = Core::ResourceType::kBuffer;
        Register();
    }


    const Core::BufferDesc& Buffer::GetDesc() const
    {
        return m_desc;
    }


    void* Buffer::Map()
    {
        FE_PROFILER_ZONE();

        FE_Assert(m_desc.m_usage == Core::ResourceUsage::kHostRandomAccess
                  || m_desc.m_usage == Core::ResourceUsage::kHostWriteThrough);

        void* result;
        VerifyVulkan(vmaMapMemory(m_vmaAllocator, m_vmaAllocation, &result));
        return result;
    }


    void Buffer::Unmap()
    {
        vmaUnmapMemory(m_vmaAllocator, m_vmaAllocation);
    }


    void Buffer::InitInternal(const VmaAllocator allocator, const Env::Name name, const Core::BufferDesc& desc)
    {
        FE_PROFILER_ZONE();

        m_name = name;
        m_desc = desc;
        m_vmaAllocator = allocator;

        VkBufferCreateInfo bufferCI{};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = desc.m_size;
        bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        if (Bit::AllSet(desc.m_flags, Core::BindFlags::kShaderResource))
        {
            bufferCI.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if (Bit::AllSet(desc.m_flags, Core::BindFlags::kUnorderedAccess))
        {
            bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
        }
        if (Bit::AllSet(desc.m_flags, Core::BindFlags::kVertexBuffer))
        {
            bufferCI.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if (Bit::AllSet(desc.m_flags, Core::BindFlags::kIndexBuffer))
        {
            bufferCI.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if (Bit::AllSet(desc.m_flags, Core::BindFlags::kConstantBuffer))
        {
            bufferCI.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        if (Bit::AllSet(desc.m_flags, Core::BindFlags::kIndirectDrawArgs))
        {
            bufferCI.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }

        VmaAllocationCreateInfo allocationCI{};
        allocationCI.usage = VMA_MEMORY_USAGE_AUTO;

        switch (desc.m_usage)
        {
        default:
            FE_DebugBreak();
            [[fallthrough]];

        case Core::ResourceUsage::kDeviceOnly:
            allocationCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;

        case Core::ResourceUsage::kHostRandomAccess:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            break;

        case Core::ResourceUsage::kHostWriteThrough:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            break;
        }

        VerifyVulkan(vmaCreateBuffer(allocator, &bufferCI, &allocationCI, &m_nativeBuffer, &m_vmaAllocation, nullptr));
        vmaSetAllocationName(allocator, m_vmaAllocation, m_name.c_str());

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_nativeBuffer);
        nameInfo.pObjectName = m_name.c_str();
        VerifyVulkan(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));
    }


    Buffer::~Buffer()
    {
        if (m_vmaAllocator != nullptr)
            vmaDestroyBuffer(m_vmaAllocator, m_nativeBuffer, m_vmaAllocation);

        m_vmaAllocator = VK_NULL_HANDLE;
        m_vmaAllocation = VK_NULL_HANDLE;
        m_nativeBuffer = VK_NULL_HANDLE;
    }
} // namespace FE::Graphics::Vulkan
