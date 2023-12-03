#include <OsGPU/Resource/VKTransientResourceHeap.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Memory/VKDeviceMemory.h>

namespace FE::Osmium
{
    Rc<IDeviceMemory> VKTransientResourceHeap::AllocateMemoryImpl()
    {
        VkMemoryRequirements requirements;
        auto* vkDevice = fe_assert_cast<VKDevice*>(m_Device);
        switch (m_Desc.TypeFlags)
        {
        case TransientResourceType::Image:
            requirements = vkDevice->GetImageMemoryRequirements();
            break;
        case TransientResourceType::Buffer:
            requirements = vkDevice->GetBufferMemoryRequirements();
            break;
        case TransientResourceType::RenderTarget:
            requirements = vkDevice->GetRenderTargetMemoryRequirements();
            break;
        default:
            FE_UNREACHABLE("Invalid TransientResourceType: {}", static_cast<Int32>(m_Desc.TypeFlags));
        }

        m_Desc.Alignment = std::max(m_Desc.Alignment, requirements.alignment);
        m_Desc.HeapSize = AlignUp(m_Desc.HeapSize, m_Desc.Alignment);

        MemoryAllocationDesc desc{};
        desc.Size = m_Desc.HeapSize;
        desc.Type = MemoryType::DeviceLocal;
        return MakeShared<VKDeviceMemory>(*vkDevice, requirements.memoryTypeBits, desc);
    }

    VKTransientResourceHeap::VKTransientResourceHeap(VKDevice& dev, const TransientResourceHeapDesc& desc)
        : TransientResourceHeapBase(dev, desc)
    {
    }

    NullableHandle VKTransientResourceHeap::AllocateResourceMemory(const BufferDesc& desc, USize& byteSize)
    {
        byteSize = desc.Size;
        return m_Allocator.Allocate(desc.Size, m_Desc.Alignment, FE_SRCPOS());
    }

    NullableHandle VKTransientResourceHeap::AllocateResourceMemory(const ImageDesc& desc, USize& byteSize)
    {
        auto* vkDevice = fe_assert_cast<VKDevice*>(m_Device);
        auto requirements = vkDevice->GetImageMemoryRequirements(desc);
        byteSize = requirements.size;
        return m_Allocator.Allocate(requirements.size, requirements.alignment, FE_SRCPOS());
    }
}
