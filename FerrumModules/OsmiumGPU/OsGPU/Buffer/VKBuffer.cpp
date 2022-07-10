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
        return m_Memory.Map(offset, size);
    }

    void VKBuffer::Unmap()
    {
        m_Memory.Unmap();
    }

    void VKBuffer::AllocateMemory(MemoryType type)
    {
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(m_Device->GetNativeDevice(), Buffer, &memoryRequirements);
        MemoryAllocationDesc desc{};
        desc.Size   = memoryRequirements.size;
        desc.Type   = type;
        auto memory = MakeShared<VKDeviceMemory>(*m_Device, memoryRequirements.memoryTypeBits, desc);
        BindMemory(DeviceMemorySlice(memory.GetRaw()));
    }

    void VKBuffer::BindMemory(const DeviceMemorySlice& memory)
    {
        m_Memory      = memory;
        auto vkMemory = fe_assert_cast<VKDeviceMemory*>(memory.Memory.GetRaw())->Memory;
        vkBindBufferMemory(m_Device->GetNativeDevice(), Buffer, vkMemory, memory.ByteOffset);
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
