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


    RHI::ResultCode Buffer::Init(StringSlice name, const RHI::BufferDesc& desc)
    {
        m_name = name;
        m_desc = desc;

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

        const VkDevice nativeDevice = NativeCast(m_device);
        if (vkCreateBuffer(nativeDevice, &bufferCI, VK_NULL_HANDLE, &m_nativeBuffer) != VK_SUCCESS)
            return RHI::ResultCode::kUnknownError;

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_nativeBuffer);
        nameInfo.pObjectName = m_name.Data();
        vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo);

        vkGetBufferMemoryRequirements(nativeDevice, m_nativeBuffer, &m_memoryRequirements);
        return RHI::ResultCode::kSuccess;
    }


    void* Buffer::Map(uint64_t offset, uint64_t size)
    {
        return m_memory.Map(offset, size);
    }


    void Buffer::Unmap()
    {
        m_memory.Unmap();
    }


    void Buffer::AllocateMemory(RHI::MemoryType type)
    {
        RHI::MemoryAllocationDesc desc{};
        desc.m_size = m_memoryRequirements.size;
        desc.m_type = type;

        DeviceMemory* memory = Rc<DeviceMemory>::DefaultNew(m_device, m_memoryRequirements.memoryTypeBits, desc);
        memory->AddRef();
        BindMemory(RHI::DeviceMemorySlice{ memory });
        m_memoryOwned = true;
    }


    void Buffer::BindMemory(const RHI::DeviceMemorySlice& memory)
    {
        m_memory = memory;
        const VkDeviceMemory vkMemory = NativeCast(memory.m_memory);
        vkBindBufferMemory(NativeCast(m_device), m_nativeBuffer, vkMemory, memory.m_byteOffset);
    }


    const RHI::BufferDesc& Buffer::GetDesc() const
    {
        return m_desc;
    }


    void Buffer::DoRelease()
    {
        if (m_memoryOwned)
            m_memory.m_memory->Release();

        DeviceObject::DoRelease();
    }


    Buffer::~Buffer()
    {
        vkDestroyBuffer(NativeCast(m_device), m_nativeBuffer, nullptr);
    }
} // namespace FE::Graphics::Vulkan
