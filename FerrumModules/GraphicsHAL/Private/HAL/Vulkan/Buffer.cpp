#include <HAL/Vulkan/Buffer.h>
#include <HAL/Vulkan/Common/Config.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/DeviceMemory.h>

namespace FE::Graphics::Vulkan
{
    Buffer::Buffer(HAL::Device* pDevice)
    {
        m_pDevice = pDevice;
        Register();
    }


    HAL::ResultCode Buffer::Init(StringSlice name, const HAL::BufferDesc& desc)
    {
        m_Name = name;
        m_Desc = desc;

        VkBufferCreateInfo bufferCI{};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = desc.Size;
        bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        if ((desc.Flags & HAL::BindFlags::ShaderResource) != HAL::BindFlags::None)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if ((desc.Flags & HAL::BindFlags::UnorderedAccess) != HAL::BindFlags::None)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            bufferCI.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
        }
        if ((desc.Flags & HAL::BindFlags::VertexBuffer) != HAL::BindFlags::None)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if ((desc.Flags & HAL::BindFlags::IndexBuffer) != HAL::BindFlags::None)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if ((desc.Flags & HAL::BindFlags::ConstantBuffer) != HAL::BindFlags::None)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        if ((desc.Flags & HAL::BindFlags::IndirectDrawArgs) != HAL::BindFlags::None)
        {
            bufferCI.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }

        const VkDevice nativeDevice = NativeCast(m_pDevice);
        if (vkCreateBuffer(nativeDevice, &bufferCI, VK_NULL_HANDLE, &m_NativeBuffer) != VK_SUCCESS)
            return HAL::ResultCode::UnknownError;

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(m_NativeBuffer);
        nameInfo.pObjectName = m_Name.Data();
        vkSetDebugUtilsObjectNameEXT(NativeCast(m_pDevice), &nameInfo);

        vkGetBufferMemoryRequirements(nativeDevice, m_NativeBuffer, &m_MemoryRequirements);
        return HAL::ResultCode::Success;
    }


    void* Buffer::Map(uint64_t offset, uint64_t size)
    {
        return m_Memory.Map(offset, size);
    }


    void Buffer::Unmap()
    {
        m_Memory.Unmap();
    }


    void Buffer::AllocateMemory(HAL::MemoryType type)
    {
        HAL::MemoryAllocationDesc desc{};
        desc.Size = m_MemoryRequirements.size;
        desc.Type = type;

        DeviceMemory* memory = Rc<DeviceMemory>::DefaultNew(m_pDevice, m_MemoryRequirements.memoryTypeBits, desc);
        memory->AddRef();
        BindMemory(HAL::DeviceMemorySlice{ memory });
        m_MemoryOwned = true;
    }


    void Buffer::BindMemory(const HAL::DeviceMemorySlice& memory)
    {
        m_Memory = memory;
        const VkDeviceMemory vkMemory = NativeCast(memory.Memory);
        vkBindBufferMemory(NativeCast(m_pDevice), m_NativeBuffer, vkMemory, memory.ByteOffset);
    }


    const HAL::BufferDesc& Buffer::GetDesc() const
    {
        return m_Desc;
    }


    void Buffer::DoRelease()
    {
        if (m_MemoryOwned)
            m_Memory.Memory->Release();

        DeviceObject::DoRelease();
    }


    Buffer::~Buffer()
    {
        vkDestroyBuffer(NativeCast(m_pDevice), m_NativeBuffer, nullptr);
    }
} // namespace FE::Graphics::Vulkan
