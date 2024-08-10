#pragma once
#include <OsGPU/Buffer/IBuffer.h>
#include <OsGPU/Common/VKConfig.h>

namespace FE::Osmium
{
    class VKDevice;
    class VKDeviceMemory;

    class VKBuffer : public IBuffer
    {
        VKDevice* m_Device;
        DeviceMemorySlice m_Memory;
        bool m_MemoryOwned = false;

    public:
        FE_RTTI_Class(VKBuffer, "CB0B65E8-B7F7-4F27-92BE-FB6E90EBD352");

        BufferDesc Desc;
        VkMemoryRequirements MemoryRequirements;
        VkBuffer Buffer;

        VKBuffer(VKDevice& dev, const BufferDesc& desc);
        ~VKBuffer() override;

        void* Map(uint64_t offset, uint64_t size) override;
        void Unmap() override;

        void AllocateMemory(MemoryType type) override;
        void BindMemory(const DeviceMemorySlice& memory) override;

        [[nodiscard]] const BufferDesc& GetDesc() const override;
    };
} // namespace FE::Osmium
