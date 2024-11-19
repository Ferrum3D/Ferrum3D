#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/DeviceMemory.h>
#include <Graphics/RHI/Vulkan/TransientResourceHeap.h>

namespace FE::Graphics::Vulkan
{
    Rc<RHI::DeviceMemory> TransientResourceHeap::AllocateMemoryImpl()
    {
        VkMemoryRequirements requirements;
        switch (m_desc.m_typeFlags)
        {
        case RHI::TransientResourceType::kImage:
            requirements = m_pMemoryRequirementsCache->GetImageMemoryRequirements();
            break;
        case RHI::TransientResourceType::kBuffer:
            requirements = m_pMemoryRequirementsCache->GetBufferMemoryRequirements();
            break;
        case RHI::TransientResourceType::kRenderTarget:
            requirements = m_pMemoryRequirementsCache->GetRenderTargetMemoryRequirements();
            break;
        default:
            FE_AssertMsg(false, "Invalid TransientResourceType: {}", static_cast<int32_t>(m_desc.m_typeFlags));
        }

        m_desc.m_alignment = std::max(m_desc.m_alignment, requirements.alignment);
        m_desc.m_heapSize = AlignUp(m_desc.m_heapSize, m_desc.m_alignment);

        RHI::MemoryAllocationDesc desc{};
        desc.m_size = m_desc.m_heapSize;
        desc.m_type = RHI::MemoryType::kDeviceLocal;
        return Rc<DeviceMemory>::DefaultNew(m_device, requirements.memoryTypeBits, desc);
    }


    TransientResourceHeap::TransientResourceHeap(RHI::Device* device, MemoryRequirementsCache* pMemoryRequirementsCache)
        : m_pMemoryRequirementsCache(pMemoryRequirementsCache)
    {
        m_device = device;
    }


    NullableHandle TransientResourceHeap::AllocateResourceMemory(const RHI::BufferDesc& desc, size_t& byteSize)
    {
        byteSize = desc.m_size;
        return m_allocator.Allocate(desc.m_size, m_desc.m_alignment);
    }


    NullableHandle TransientResourceHeap::AllocateResourceMemory(const RHI::ImageDesc& desc, size_t& byteSize)
    {
        const auto requirements = m_pMemoryRequirementsCache->GetImageMemoryRequirements(desc);
        byteSize = requirements.size;
        return m_allocator.Allocate(requirements.size, requirements.alignment);
    }
} // namespace FE::Graphics::Vulkan
