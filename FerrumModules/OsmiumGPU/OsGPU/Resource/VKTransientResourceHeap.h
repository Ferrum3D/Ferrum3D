#pragma once
#include <OsGPU/Resource/TransientResourceHeapBase.h>

namespace FE::Osmium
{
    class VKDevice;

    class VKTransientResourceHeap : public TransientResourceHeapBase
    {
    protected:
        Rc<IDeviceMemory> AllocateMemoryImpl() override;
        NullableHandle AllocateResourceMemory(const BufferDesc& desc, USize& byteSize) override;
        NullableHandle AllocateResourceMemory(const ImageDesc& desc, USize& byteSize) override;

    public:
        FE_CLASS_RTTI(VKTransientResourceHeap, "CEE0A24C-3F26-4B18-B4C9-EA9566635D9E");

        VKTransientResourceHeap(VKDevice& dev, const TransientResourceHeapDesc& desc);
        ~VKTransientResourceHeap() override = default;
    };
} // namespace FE::Osmium
