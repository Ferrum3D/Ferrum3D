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
        inline static constexpr uint64_t SemaphoreTimeout = uint64_t(-1);
        vk::PipelineStageFlags Flags = vk::PipelineStageFlagBits::eAllCommands;

        VKFence(VKDevice& dev, uint64_t value);

        virtual void Wait(uint64_t value) override;
        virtual void Signal(uint64_t value) override;

        vk::Semaphore& GetNativeSemaphore();
    };
}
