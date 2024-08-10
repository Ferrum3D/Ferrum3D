#pragma once
#include <OsGPU/Resource/TransientResourceHeapBase.h>

namespace FE::Osmium
{
    class VKDevice;

    class VKTransientResourceHeap : public TransientResourceHeapBase
    {
    protected:
        Rc<IDeviceMemory> AllocateMemoryImpl() override;
        NullableHandle AllocateResourceMemory(const BufferDesc& desc, size_t& byteSize) override;
        NullableHandle AllocateResourceMemory(const ImageDesc& desc, size_t& byteSize) override;

    public:
        FE_RTTI_Class(VKTransientResourceHeap, "CEE0A24C-3F26-4B18-B4C9-EA9566635D9E");

        VKTransientResourceHeap(VKDevice& dev, const TransientResourceHeapDesc& desc);
        ~VKTransientResourceHeap() override = default;
    };
} // namespace FE::Osmium
