#pragma once
#include <FeGPU/Common/VKConfig.h>
#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Fence/VKFence.h>

namespace FE::GPU
{
    VKFence::VKFence(VKDevice& dev, UInt64 value)
    {
        vk::SemaphoreTypeCreateInfo timelineCI{};
        timelineCI.initialValue  = value;
        timelineCI.semaphoreType = vk::SemaphoreType::eTimeline;
        vk::SemaphoreCreateInfo semaphoreCI;
        semaphoreCI.pNext = &timelineCI;
        m_Semaphore       = dev.GetNativeDevice().createSemaphoreUnique(semaphoreCI);
        m_Device          = &dev;
    }

    void VKFence::Wait(UInt64 value)
    {
        vk::SemaphoreWaitInfo waitInfo{};
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores    = &m_Semaphore.get();
        waitInfo.pValues        = &value;
        FE_VK_ASSERT(m_Device->GetNativeDevice().waitSemaphoresKHR(waitInfo, SemaphoreTimeout));
    }

    void VKFence::Signal(UInt64 value)
    {
        vk::SemaphoreSignalInfo signalInfo{};
        signalInfo.semaphore = m_Semaphore.get();
        signalInfo.value     = value;
        m_Device->GetNativeDevice().signalSemaphoreKHR(signalInfo);
    }

    vk::Semaphore& VKFence::GetNativeSemaphore()
    {
        return m_Semaphore.get();
    }
} // namespace FE::GPU
