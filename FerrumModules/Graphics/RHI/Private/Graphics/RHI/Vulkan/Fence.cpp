#include <Graphics/RHI/Vulkan/CommandQueue.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/Fence.h>

namespace FE::Graphics::Vulkan
{
    Fence::Fence(RHI::Device* device)
    {
        m_device = device;
    }


    RHI::ResultCode Fence::Init(uint64_t initialValue)
    {
        VkSemaphoreTypeCreateInfo typeCI{};
        typeCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        typeCI.initialValue = initialValue;
        typeCI.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;

        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCI.pNext = &typeCI;

        FE_VK_ASSERT(vkCreateSemaphore(NativeCast(m_device), &semaphoreCI, nullptr, &m_timelineSemaphore));
        return RHI::ResultCode::kSuccess;
    }


    void Fence::Signal(uint64_t value)
    {
        VkSemaphoreSignalInfo signalInfo{};
        signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
        signalInfo.value = value;
        signalInfo.semaphore = m_timelineSemaphore;
        FE_VK_ASSERT(vkSignalSemaphore(NativeCast(m_device), &signalInfo));
    }


    void Fence::Wait(uint64_t value)
    {
        VkSemaphoreWaitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &m_timelineSemaphore;
        waitInfo.pValues = &value;
        FE_VK_ASSERT(vkWaitSemaphores(NativeCast(m_device), &waitInfo, UINT64_MAX));
    }


    uint64_t Fence::GetCompletedValue()
    {
        uint64_t result;
        FE_VK_ASSERT(vkGetSemaphoreCounterValue(NativeCast(m_device), m_timelineSemaphore, &result));
        return result;
    }


    Fence::~Fence()
    {
        vkDestroySemaphore(NativeCast(m_device), m_timelineSemaphore, nullptr);
    }
} // namespace FE::Graphics::Vulkan
