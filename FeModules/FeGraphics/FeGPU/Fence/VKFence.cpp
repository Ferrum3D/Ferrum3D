#pragma once
#include <FeGPU/Common/VKConfig.h>
#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Fence/VKFence.h>

namespace FE::GPU
{
    VKFence::VKFence(VKDevice& dev, uint64_t value)
    {
        vk::SemaphoreTypeCreateInfo timelineCI{};
        timelineCI.initialValue  = value;
        timelineCI.semaphoreType = vk::SemaphoreType::eTimeline;
        vk::SemaphoreCreateInfo semaphoreCI;
        semaphoreCI.pNext = &timelineCI;
        m_Semaphore       = dev.GetNativeDevice().createSemaphoreUnique(semaphoreCI);
        m_Device          = &dev;
    }

    void VKFence::Wait(uint64_t value)
    {
        vk::SemaphoreWaitInfo waitInfo{};
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores    = &m_Semaphore.get();
        waitInfo.pValues        = &value;
        m_Device->GetNativeDevice().waitSemaphoresKHR(waitInfo, uint64_t(-1));
    }

    void VKFence::Signal(uint64_t value)
    {
        vk::SemaphoreSignalInfo signalInfo{};
        signalInfo.semaphore = m_Semaphore.get();
        signalInfo.value     = value;
        m_Device->GetNativeDevice().signalSemaphoreKHR(signalInfo);
    }
} // namespace FE::GPU
