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
        MemoryAllocationDesc desc{};
        desc.Size   = MemoryRequirements.size;
        desc.Type   = type;
        auto memory = MakeShared<VKDeviceMemory>(*m_Device, MemoryRequirements.memoryTypeBits, desc);
        BindMemory(DeviceMemorySlice(memory.Detach()));
        m_MemoryOwned = true;
    }

    void VKBuffer::BindMemory(const DeviceMemorySlice& memory)
    {
        m_Memory      = memory;
        auto vkMemory = fe_assert_cast<VKDeviceMemory*>(memory.Memory)->Memory;
        vkBindBufferMemory(m_Device->GetNativeDevice(), Buffer, vkMemory, memory.ByteOffset);
    }

    const BufferDesc& VKBuffer::GetDesc() const
    {
        return Desc;
    }

    FE_VK_OBJECT_DELETER(Buffer);

    VKBuffer::~VKBuffer()
    {
        FE_DELETE_VK_OBJECT(Buffer, Buffer);
        if (m_MemoryOwned)
        {
            m_Memory.Memory->ReleaseStrongRef();
        }
    }
} // namespace FE::Osmium
