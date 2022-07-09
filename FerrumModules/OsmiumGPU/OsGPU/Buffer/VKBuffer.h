#pragma once
#include <OsGPU/Buffer/IBuffer.h>
#include <OsGPU/Common/VKConfig.h>

namespace FE::Osmium
{
    class VKDevice;
    class VKDeviceMemory;

    class VKBuffer : public Object<IBuffer>
    {
        VKDevice* m_Device;
        Shared<VKDeviceMemory> m_Memory;

    public:
        FE_CLASS_RTTI(VKBuffer, "CB0B65E8-B7F7-4F27-92BE-FB6E90EBD352");

        BufferDesc Desc;
        VkBuffer Buffer;

        VKBuffer(VKDevice& dev, const BufferDesc& desc);
        ~VKBuffer() override;

        void* Map(UInt64 offset, UInt64 size) override;
        void Unmap() override;

        void AllocateMemory(MemoryType type) override;
        void BindMemory(const Shared<IDeviceMemory>& memory, UInt64 offset) override;

        [[nodiscard]] const BufferDesc& GetDesc() const override;
    };
} // namespace FE::Osmium
