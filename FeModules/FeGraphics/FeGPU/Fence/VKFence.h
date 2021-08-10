#pragma once
#include <FeGPU/Fence/IFence.h>

namespace FE::GPU
{
    class VKDevice;

    class VKFence : public Object<IFence>
    {
        vk::UniqueSemaphore m_Semaphore;
        VKDevice* m_Device;

    public:
        FE_CLASS_RTTI(VKFence, "78363647-3381-46F2-97B1-2A1AC8AFC3C1");

        inline static constexpr UInt64 SemaphoreTimeout = UInt64(-1);
        vk::PipelineStageFlags Flags = vk::PipelineStageFlagBits::eAllCommands;

        VKFence(VKDevice& dev, UInt64 value);

        virtual void Wait(UInt64 value) override;
        virtual void Signal(UInt64 value) override;

        vk::Semaphore& GetNativeSemaphore();
    };
}
