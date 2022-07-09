#include <OsGPU/Buffer/VKBuffer.h>
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Memory/VKDeviceMemory.h>

namespace FE::Osmium
{
    VKBuffer::VKBuffer(VKDevice& dev, const BufferDesc& desc)
        : m_Device(&dev)
        , Desc(desc)
    {
    }

    void* VKBuffer::Map(UInt64 offset, UInt64 size)
    {
        void* data;
        FE_VK_ASSERT(vkMapMemory(m_Device->GetNativeDevice(), m_Memory->Memory, offset, size, 0, &data));
        return data;
    }

    void VKBuffer::Unmap()
    {
        vkUnmapMemory(m_Device->GetNativeDevice(), m_Memory->Memory);
    }

    void VKBuffer::AllocateMemory(MemoryType type)
    {
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(m_Device->GetNativeDevice(), Buffer, &memoryRequirements);
        MemoryAllocationDesc desc{};
        desc.Size = memoryRequirements.size;
        desc.Type = type;
        m_Memory  = MakeShared<VKDeviceMemory>(*m_Device, memoryRequirements.memoryTypeBits, desc);
        BindMemory(m_Memory, 0);
    }

    void VKBuffer::BindMemory(const Shared<IDeviceMemory>& memory, UInt64 offset)
    {
        m_Memory = static_pointer_cast<VKDeviceMemory>(memory);
        vkBindBufferMemory(m_Device->GetNativeDevice(), Buffer, m_Memory->Memory, offset);
    }

    const BufferDesc& VKBuffer::GetDesc() const
    {
        return Desc;
    }

    VKBuffer::~VKBuffer()
    {
        vkDestroyBuffer(m_Device->GetNativeDevice(), Buffer, VK_NULL_HANDLE);
    }
} // namespace FE::Osmium
