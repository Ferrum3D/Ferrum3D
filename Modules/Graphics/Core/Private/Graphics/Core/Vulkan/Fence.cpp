#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Fence.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        VulkanObjectPoolType GFencePool{ "VulkanFencePool", sizeof(Fence) };
    }


    Fence* Fence::Create(Core::Device* device)
    {
        FE_PROFILER_ZONE();

        return Rc<Fence>::Allocate(&GFencePool, [device](void* memory) {
            return new (memory) Fence(device);
        });
    }


    Fence::Fence(Core::Device* device)
    {
        m_device = device;
    }


    Core::ResultCode Fence::Init(const uint64_t initialValue)
    {
        FE_PROFILER_ZONE();

        VkSemaphoreTypeCreateInfo typeCI{};
        typeCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        typeCI.initialValue = initialValue;
        typeCI.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;

        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCI.pNext = &typeCI;

        VerifyVulkan(vkCreateSemaphore(NativeCast(m_device), &semaphoreCI, nullptr, &m_timelineSemaphore));
        return Core::ResultCode::kSuccess;
    }


    void Fence::Signal(const uint64_t value)
    {
        FE_PROFILER_ZONE();

        VkSemaphoreSignalInfo signalInfo{};
        signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
        signalInfo.value = value;
        signalInfo.semaphore = m_timelineSemaphore;
        VerifyVulkan(vkSignalSemaphore(NativeCast(m_device), &signalInfo));
    }


    void Fence::Wait(const uint64_t value)
    {
        FE_PROFILER_ZONE();

        VkSemaphoreWaitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &m_timelineSemaphore;
        waitInfo.pValues = &value;
        VerifyVulkan(vkWaitSemaphores(NativeCast(m_device), &waitInfo, Constants::kMaxU64));
    }


    uint64_t Fence::GetCompletedValue()
    {
        uint64_t result;
        VerifyVulkan(vkGetSemaphoreCounterValue(NativeCast(m_device), m_timelineSemaphore, &result));
        return result;
    }


    Fence::~Fence()
    {
        if (m_timelineSemaphore)
            vkDestroySemaphore(NativeCast(m_device), m_timelineSemaphore, nullptr);
    }
} // namespace FE::Graphics::Vulkan
