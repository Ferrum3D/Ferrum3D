#include <OsGPU/Buffer/VKBuffer.h>
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Memory/VKDeviceMemory.h>

namespace FE::GPU
{
    VKBuffer::VKBuffer(VKDevice& dev, const BufferDesc& desc)
        : m_Device(&dev)
        , Desc(desc)
    {
    }

    void* VKBuffer::Map(UInt64 offset, UInt64 size)
    {
        return m_Device->GetNativeDevice().mapMemory(m_Memory->Memory.get(), offset, size);
    }

    void VKBuffer::Unmap()
    {
        m_Device->GetNativeDevice().unmapMemory(m_Memory->Memory.get());
    }

    void VKBuffer::AllocateMemory(MemoryType type)
    {
        auto memoryRequirements = m_Device->GetNativeDevice().getBufferMemoryRequirements(Buffer.get());
        MemoryAllocationDesc desc{};
        desc.Size = memoryRequirements.size;
        desc.Type = type;
        m_Memory  = MakeShared<VKDeviceMemory>(*m_Device, memoryRequirements.memoryTypeBits, desc);
        BindMemory(static_pointer_cast<IDeviceMemory>(m_Memory), 0);
    }

    void VKBuffer::BindMemory(const Shared<IDeviceMemory>& memory, UInt64 offset)
    {
        m_Memory = static_pointer_cast<VKDeviceMemory>(memory);
        m_Device->GetNativeDevice().bindBufferMemory(Buffer.get(), m_Memory->Memory.get(), offset);
    }

    const BufferDesc& VKBuffer::GetDesc() const
    {
        return Desc;
    }
} // namespace FE::GPU
