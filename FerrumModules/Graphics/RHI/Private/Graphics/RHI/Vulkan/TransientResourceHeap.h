#pragma once
#include <Graphics/RHI/TransientResourceHeapBase.h>
#include <Graphics/RHI/Vulkan/MemoryRequirementsCache.h>

namespace FE::Graphics::Vulkan
{
    struct TransientResourceHeap final : public RHI::TransientResourceHeapBase
    {
        FE_RTTI_Class(TransientResourceHeap, "CEE0A24C-3F26-4B18-B4C9-EA9566635D9E");

        TransientResourceHeap(RHI::Device* device, MemoryRequirementsCache* pMemoryRequirementsCache);
        ~TransientResourceHeap() override = default;

    private:
        Rc<MemoryRequirementsCache> m_pMemoryRequirementsCache = nullptr;

    protected:
        Rc<RHI::DeviceMemory> AllocateMemoryImpl() override;
        NullableHandle AllocateResourceMemory(const RHI::BufferDesc& desc, size_t& byteSize) override;
        NullableHandle AllocateResourceMemory(const RHI::ImageDesc& desc, size_t& byteSize) override;
    };

    FE_ENABLE_IMPL_CAST(TransientResourceHeap);
} // namespace FE::Graphics::Vulkan
