#pragma once
#include <FeGPU/Common/VKConfig.h>
#include <FeGPU/Memory/IDeviceMemory.h>

namespace FE::GPU
{
    class VKDevice;

    class VKDeviceMemory : public Object<IDeviceMemory>
    {
        MemoryAllocationDesc m_Desc;

    public:
        vk::UniqueDeviceMemory Memory;

        FE_CLASS_RTTI(VKDeviceMemory, "D80E7CF1-4D15-4AEB-8CDA-5275195BC389");

        VKDeviceMemory(VKDevice& dev, UInt32 typeBits, const MemoryAllocationDesc& desc);

        virtual const MemoryAllocationDesc& GetDesc() override;
    };
} // namespace FE::GPU
