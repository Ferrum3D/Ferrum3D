#include <HAL/Vulkan/CommandQueue.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/Fence.h>

namespace FE::Graphics::Vulkan
{
    Fence::Fence(HAL::Device* pDevice)
    {
        m_pDevice = pDevice;
    }


    HAL::ResultCode Fence::Init(uint64_t initialValue)
    {
        VkSemaphoreTypeCreateInfo typeCI{};
        typeCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        typeCI.initialValue = initialValue;
        typeCI.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;

        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCI.pNext = &typeCI;

        FE_VK_ASSERT(vkCreateSemaphore(NativeCast(m_pDevice), &semaphoreCI, nullptr, &m_timelineSemaphore));
        return HAL::ResultCode::Success;
    }


    void Fence::Signal(uint64_t value)
    {
        VkSemaphoreSignalInfo signalInfo{};
        signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
        signalInfo.value = value;
        signalInfo.semaphore = m_timelineSemaphore;
        FE_VK_ASSERT(vkSignalSemaphore(NativeCast(m_pDevice), &signalInfo));
    }


    void Fence::Wait(uint64_t value)
    {
        VkSemaphoreWaitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &m_timelineSemaphore;
        waitInfo.pValues = &value;
        FE_VK_ASSERT(vkWaitSemaphores(NativeCast(m_pDevice), &waitInfo, UINT64_MAX));
    }


    uint64_t Fence::GetCompletedValue()
    {
        uint64_t result;
        FE_VK_ASSERT(vkGetSemaphoreCounterValue(NativeCast(m_pDevice), m_timelineSemaphore, &result));
        return result;
    }


    Fence::~Fence()
    {
        vkDestroySemaphore(NativeCast(m_pDevice), m_timelineSemaphore, nullptr);
    }
} // namespace FE::Graphics::Vulkan
