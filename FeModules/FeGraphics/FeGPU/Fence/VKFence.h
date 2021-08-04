#pragma once
#include <FeGPU/Fence/IFence.h>

namespace FE::GPU
{
    class VKDevice;

    class VKFence : public IFence
    {
        vk::UniqueSemaphore m_Semaphore;
        VKDevice* m_Device;

    public:
        VKFence(VKDevice& dev, uint64_t value);

        virtual void Wait(uint64_t value) override;
        virtual void Signal(uint64_t value) override;
    };
}
