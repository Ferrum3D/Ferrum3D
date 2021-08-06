#pragma once
#include <FeGPU/Common/VKConfig.h>
#include <FeGPU/Memory/IDeviceMemory.h>

namespace FE::GPU
{
    class VKDevice;

    class VKDeviceMemory : public IDeviceMemory
    {
        MemoryAllocationDesc m_Desc;

    public:
        vk::UniqueDeviceMemory Memory;

        VKDeviceMemory(VKDevice& dev, UInt32 typeBits, const MemoryAllocationDesc& desc);

        virtual const MemoryAllocationDesc& GetDesc() override;
    };
} // namespace FE::GPU
