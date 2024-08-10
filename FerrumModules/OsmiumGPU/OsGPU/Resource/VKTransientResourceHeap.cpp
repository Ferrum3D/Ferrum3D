#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Memory/VKDeviceMemory.h>
#include <OsGPU/Resource/VKTransientResourceHeap.h>

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
            FE_UNREACHABLE("Invalid TransientResourceType: {}", static_cast<int32_t>(m_Desc.TypeFlags));
        }

        m_Desc.Alignment = std::max(m_Desc.Alignment, requirements.alignment);
        m_Desc.HeapSize = AlignUp(m_Desc.HeapSize, m_Desc.Alignment);

        MemoryAllocationDesc desc{};
        desc.Size = m_Desc.HeapSize;
        desc.Type = MemoryType::DeviceLocal;
        return Rc<VKDeviceMemory>::DefaultNew(*vkDevice, requirements.memoryTypeBits, desc);
    }

    VKTransientResourceHeap::VKTransientResourceHeap(VKDevice& dev, const TransientResourceHeapDesc& desc)
        : TransientResourceHeapBase(dev, desc)
    {
    }

    NullableHandle VKTransientResourceHeap::AllocateResourceMemory(const BufferDesc& desc, size_t& byteSize)
    {
        byteSize = desc.Size;
        return m_Allocator.Allocate(desc.Size, m_Desc.Alignment);
    }

    NullableHandle VKTransientResourceHeap::AllocateResourceMemory(const ImageDesc& desc, size_t& byteSize)
    {
        auto* vkDevice = fe_assert_cast<VKDevice*>(m_Device);
        auto requirements = vkDevice->GetImageMemoryRequirements(desc);
        byteSize = requirements.size;
        return m_Allocator.Allocate(requirements.size, requirements.alignment);
    }
} // namespace FE::Osmium
