#include <Graphics/RHI/Vulkan/Buffer.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/DeviceMemory.h>

namespace FE::Graphics::Vulkan
{
    Buffer::Buffer(RHI::Device* device)
    {
        m_device = device;
        Register();
    }


    RHI::ResultCode Buffer::InitInternal(VmaAllocator allocator, Env::Name name, const RHI::BufferDesc& desc)
    {
        m_name = name;
        m_desc = desc;
        m_vmaAllocator = allocator;

        VkBufferCreateInfo bufferCI{};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = desc.m_size;
        bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        if ((desc.m_flags & RHI::BindFlags::kShaderResource) != RHI::BindFlags::kNone)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if ((desc.m_flags & RHI::BindFlags::kUnorderedAccess) != RHI::BindFlags::kNone)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
        }
        if ((desc.m_flags & RHI::BindFlags::kVertexBuffer) != RHI::BindFlags::kNone)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if ((desc.m_flags & RHI::BindFlags::kIndexBuffer) != RHI::BindFlags::kNone)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if ((desc.m_flags & RHI::BindFlags::kConstantBuffer) != RHI::BindFlags::kNone)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        if ((desc.m_flags & RHI::BindFlags::kIndirectDrawArgs) != RHI::BindFlags::kNone)
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

        case RHI::ResourceUsage::kDeviceOnly:
            allocationCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;

        case RHI::ResourceUsage::kHostRandomAccess:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            break;

        case RHI::ResourceUsage::kHostWriteThrough:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            break;
        }

        if (vmaCreateBuffer(allocator, &bufferCI, &allocationCI, &m_nativeBuffer, &m_vmaAllocation, nullptr) != VK_SUCCESS)
            return RHI::ResultCode::kUnknownError;

        vmaSetAllocationName(allocator, m_vmaAllocation, m_name.c_str());

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_nativeBuffer);
        nameInfo.pObjectName = m_name.c_str();
        if (vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo) != VK_SUCCESS)
            return RHI::ResultCode::kUnknownError;

        return RHI::ResultCode::kSuccess;
    }


    void* Buffer::Map(uint32_t, uint32_t)
    {
        FE_AssertDebug(m_desc.m_usage == RHI::ResourceUsage::kHostRandomAccess
                       || m_desc.m_usage == RHI::ResourceUsage::kHostWriteThrough);

        void* result;
        FE_VK_ASSERT(vmaMapMemory(m_vmaAllocator, m_vmaAllocation, &result));
        return result;
    }


    void Buffer::Unmap()
    {
        vmaUnmapMemory(m_vmaAllocator, m_vmaAllocation);
    }


    const RHI::BufferDesc& Buffer::GetDesc() const
    {
        return m_desc;
    }


    Buffer::~Buffer()
    {
        vkDestroyBuffer(NativeCast(m_device), m_nativeBuffer, nullptr);
    }
} // namespace FE::Graphics::Vulkan
