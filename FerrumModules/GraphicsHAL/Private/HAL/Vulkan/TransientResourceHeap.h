#pragma once
#include <HAL/TransientResourceHeapBase.h>
#include <HAL/Vulkan/MemoryRequirementsCache.h>

namespace FE::Graphics::Vulkan
{
    class TransientResourceHeap final : public HAL::TransientResourceHeapBase
    {
        Rc<MemoryRequirementsCache> m_pMemoryRequirementsCache = nullptr;

    protected:
        Rc<HAL::DeviceMemory> AllocateMemoryImpl() override;
        NullableHandle AllocateResourceMemory(const HAL::BufferDesc& desc, size_t& byteSize) override;
        NullableHandle AllocateResourceMemory(const HAL::ImageDesc& desc, size_t& byteSize) override;

    public:
        FE_RTTI_Class(TransientResourceHeap, "CEE0A24C-3F26-4B18-B4C9-EA9566635D9E");

        TransientResourceHeap(HAL::Device* pDevice, MemoryRequirementsCache* pMemoryRequirementsCache);
        ~TransientResourceHeap() override = default;
    };

    FE_ENABLE_IMPL_CAST(TransientResourceHeap);
} // namespace FE::Graphics::Vulkan
