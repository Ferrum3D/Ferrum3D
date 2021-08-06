#include <FeGPU/Buffer/VKBuffer.h>
#include <FeGPU/Common/VKConfig.h>
#include <FeGPU/Device/VKDevice.h>

namespace FE::GPU
{
    VKBuffer::VKBuffer(VKDevice& dev, const BufferDesc& desc)
        : m_Device(&dev)
        , Desc(desc)
    {
    }

    void* VKBuffer::Map(uint64_t offset, uint64_t size)
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
        BindMemory(StaticPtrCast<IDeviceMemory>(m_Memory), 0);
    }

    void VKBuffer::BindMemory(const RefCountPtr<IDeviceMemory>& memory, uint64_t offset)
    {
        m_Memory = StaticPtrCast<VKDeviceMemory>(memory);
        m_Device->GetNativeDevice().bindBufferMemory(Buffer.get(), m_Memory->Memory.get(), offset);
    }
} // namespace FE::GPU
