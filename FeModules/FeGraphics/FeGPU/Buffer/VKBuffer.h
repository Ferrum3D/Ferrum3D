#pragma once
#include <FeGPU/Buffer/IBuffer.h>
#include <FeGPU/Memory/VKDeviceMemory.h>
#include <FeGPU/Common/VKConfig.h>

namespace FE::GPU
{
    class VKBuffer : public Object<IBuffer>
    {
        VKDevice* m_Device;
        RefCountPtr<VKDeviceMemory> m_Memory;

    public:
        FE_CLASS_RTTI(VKBuffer, "CB0B65E8-B7F7-4F27-92BE-FB6E90EBD352");

        BufferDesc Desc;
        vk::UniqueBuffer Buffer;

        VKBuffer(VKDevice& dev, const BufferDesc& desc);

        virtual void* Map(UInt64 offset, UInt64 size = static_cast<UInt64>(-1)) override;
        virtual void Unmap() override;

        virtual void AllocateMemory(MemoryType type) override;
        virtual void BindMemory(const RefCountPtr<IDeviceMemory>& memory, UInt64 offset) override;
    };
} // namespace FE::GPU
