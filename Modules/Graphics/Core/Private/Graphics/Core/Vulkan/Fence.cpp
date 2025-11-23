#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Fence.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        VulkanObjectPoolType GFencePool{ "VulkanFencePool", sizeof(Fence) };
    }


    Fence* Fence::Create(Core::Device* device, uint64_t initialValue)
    {
        FE_PROFILER_ZONE();

        return Rc<Fence>::Allocate(&GFencePool, [device, initialValue](void* memory) {
            return new (memory) Fence(device, initialValue);
        });
    }


    Fence::Fence(Core::Device* device, const uint64_t initialValue)
    {
        FE_PROFILER_ZONE();

        m_device = device;

        VkSemaphoreTypeCreateInfo typeCI{};
        typeCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        typeCI.initialValue = initialValue;
        typeCI.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;

        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCI.pNext = &typeCI;

        VerifyVk(vkCreateSemaphore(NativeCast(m_device), &semaphoreCI, nullptr, &m_timelineSemaphore));
    }


    void Fence::Signal(const uint64_t value)
    {
        FE_PROFILER_ZONE();

        VkSemaphoreSignalInfo signalInfo{};
        signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
        signalInfo.value = value;
        signalInfo.semaphore = m_timelineSemaphore;
        VerifyVk(vkSignalSemaphore(NativeCast(m_device), &signalInfo));
    }


    void Fence::Wait(const uint64_t value)
    {
        FE_PROFILER_ZONE();

        VkSemaphoreWaitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &m_timelineSemaphore;
        waitInfo.pValues = &value;
        VerifyVk(vkWaitSemaphores(NativeCast(m_device), &waitInfo, Constants::kMaxU64));
    }


    uint64_t Fence::GetCompletedValue()
    {
        uint64_t result;
        VerifyVk(vkGetSemaphoreCounterValue(NativeCast(m_device), m_timelineSemaphore, &result));
        return result;
    }


    Fence::~Fence()
    {
        if (m_timelineSemaphore)
            vkDestroySemaphore(NativeCast(m_device), m_timelineSemaphore, nullptr);
    }
} // namespace FE::Graphics::Vulkan
