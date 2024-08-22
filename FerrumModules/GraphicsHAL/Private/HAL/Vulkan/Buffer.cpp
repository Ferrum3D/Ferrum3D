#include <HAL/Vulkan/Buffer.h>
#include <HAL/Vulkan/Common/Config.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/DeviceMemory.h>

namespace FE::Graphics::Vulkan
{
    Buffer::Buffer(HAL::Device* pDevice)
    {
        m_pDevice = pDevice;
    }


    HAL::ResultCode Buffer::Init(const HAL::BufferDesc& desc)
    {
        Desc = desc;

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

        const VkDevice nativeDevice = ImplCast(m_pDevice)->GetNativeDevice();
        if (vkCreateBuffer(nativeDevice, &bufferCI, VK_NULL_HANDLE, &NativeBuffer) != VK_SUCCESS)
            return HAL::ResultCode::UnknownError;

        vkGetBufferMemoryRequirements(nativeDevice, NativeBuffer, &MemoryRequirements);
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
        desc.Size = MemoryRequirements.size;
        desc.Type = type;

        DeviceMemory* memory = Rc<DeviceMemory>::DefaultNew(m_pDevice, MemoryRequirements.memoryTypeBits, desc);
        memory->AddRef();
        BindMemory(HAL::DeviceMemorySlice{ memory });
        m_MemoryOwned = true;
    }


    void Buffer::BindMemory(const HAL::DeviceMemorySlice& memory)
    {
        m_Memory = memory;
        const VkDeviceMemory vkMemory = ImplCast(memory.Memory)->Memory;
        vkBindBufferMemory(ImplCast(m_pDevice)->GetNativeDevice(), NativeBuffer, vkMemory, memory.ByteOffset);
    }


    const HAL::BufferDesc& Buffer::GetDesc() const
    {
        return Desc;
    }


    void Buffer::DoRelease()
    {
        if (m_MemoryOwned)
            m_Memory.Memory->Release();

        DeviceObject::DoRelease();
    }


    Buffer::~Buffer()
    {
        vkDestroyBuffer(ImplCast(m_pDevice)->GetNativeDevice(), NativeBuffer, nullptr);
    }
} // namespace FE::Graphics::Vulkan
