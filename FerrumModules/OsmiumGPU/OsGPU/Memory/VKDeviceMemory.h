#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Memory/IDeviceMemory.h>

namespace FE::Osmium
{
    class VKDevice;

    class VKDeviceMemory : public IDeviceMemory
    {
        VKDevice* m_Device;
        MemoryAllocationDesc m_Desc;

    public:
        VkDeviceMemory Memory;

        FE_RTTI_Class(VKDeviceMemory, "D80E7CF1-4D15-4AEB-8CDA-5275195BC389");

        VKDeviceMemory(VKDevice& dev, uint32_t typeBits, const MemoryAllocationDesc& desc);
        ~VKDeviceMemory() override;

        void* Map(size_t offset, size_t size) override;
        void Unmap() override;
        const MemoryAllocationDesc& GetDesc() override;
    };
} // namespace FE::Osmium
