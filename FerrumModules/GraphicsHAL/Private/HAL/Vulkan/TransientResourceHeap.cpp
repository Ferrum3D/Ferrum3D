#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/DeviceMemory.h>
#include <HAL/Vulkan/TransientResourceHeap.h>

namespace FE::Graphics::Vulkan
{
    Rc<HAL::DeviceMemory> TransientResourceHeap::AllocateMemoryImpl()
    {
        VkMemoryRequirements requirements;
        switch (m_Desc.TypeFlags)
        {
        case HAL::TransientResourceType::Image:
            requirements = m_pMemoryRequirementsCache->GetImageMemoryRequirements();
            break;
        case HAL::TransientResourceType::Buffer:
            requirements = m_pMemoryRequirementsCache->GetBufferMemoryRequirements();
            break;
        case HAL::TransientResourceType::RenderTarget:
            requirements = m_pMemoryRequirementsCache->GetRenderTargetMemoryRequirements();
            break;
        default:
            FE_AssertMsg(false, "Invalid TransientResourceType: {}", static_cast<int32_t>(m_Desc.TypeFlags));
        }

        m_Desc.Alignment = std::max(m_Desc.Alignment, requirements.alignment);
        m_Desc.HeapSize = AlignUp(m_Desc.HeapSize, m_Desc.Alignment);

        HAL::MemoryAllocationDesc desc{};
        desc.Size = m_Desc.HeapSize;
        desc.Type = HAL::MemoryType::kDeviceLocal;
        return Rc<DeviceMemory>::DefaultNew(m_pDevice, requirements.memoryTypeBits, desc);
    }


    TransientResourceHeap::TransientResourceHeap(HAL::Device* pDevice, MemoryRequirementsCache* pMemoryRequirementsCache)
        : m_pMemoryRequirementsCache(pMemoryRequirementsCache)
    {
        m_pDevice = pDevice;
    }


    NullableHandle TransientResourceHeap::AllocateResourceMemory(const HAL::BufferDesc& desc, size_t& byteSize)
    {
        byteSize = desc.Size;
        return m_Allocator.Allocate(desc.Size, m_Desc.Alignment);
    }


    NullableHandle TransientResourceHeap::AllocateResourceMemory(const HAL::ImageDesc& desc, size_t& byteSize)
    {
        const auto requirements = m_pMemoryRequirementsCache->GetImageMemoryRequirements(desc);
        byteSize = requirements.size;
        return m_Allocator.Allocate(requirements.size, requirements.alignment);
    }
} // namespace FE::Graphics::Vulkan
