#pragma once
#include <FeGPU/Descriptors/IDescriptorHeap.h>
#include <FeGPU/Common/VKConfig.h>

namespace FE::GPU
{
    class VKDevice;

    class VKDescriptorHeap : public Object<IDescriptorHeap>
    {
        vk::UniqueDescriptorPool m_NativePool;
        VKDevice* m_Device;

    public:
        FE_CLASS_RTTI(VKDescriptorHeap, "5AFA0C8B-35EE-4B53-9144-C3BD5A8AA51D");

        VKDescriptorHeap(VKDevice& dev, const DescriptorHeapDesc& desc);

        vk::DescriptorPool& GetNativeDescriptorPool();
    };
}
