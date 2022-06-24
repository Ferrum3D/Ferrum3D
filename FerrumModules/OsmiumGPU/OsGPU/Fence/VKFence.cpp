#include <OsGPU/CommandQueue/VKCommandQueue.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Fence/VKFence.h>

namespace FE::Osmium
{
    VKFence::VKFence(VKDevice& dev, FenceState initialState)
        : m_Device(&dev)
    {
        vk::FenceCreateInfo fenceCI{};
        fenceCI.flags =
            initialState == FenceState::Reset ? static_cast<vk::FenceCreateFlagBits>(0) : vk::FenceCreateFlagBits::eSignaled;
        m_NativeFence = m_Device->GetNativeDevice().createFenceUnique(fenceCI);
    }

    void VKFence::SignalOnCPU()
    {
        auto queue = m_Device->GetCommandQueue(CommandQueueClass::Graphics);
        queue->SignalFence(this);
    }

    void VKFence::WaitOnCPU()
    {
        FE_VK_ASSERT(m_Device->GetNativeDevice().waitForFences({ m_NativeFence.get() }, false, static_cast<UInt64>(-1)));
    }

    void VKFence::Reset()
    {
        m_Device->GetNativeDevice().resetFences({ m_NativeFence.get() });
    }

    FenceState VKFence::GetState()
    {
        return FenceState::Signaled;
    }

    vk::Fence& VKFence::GetNativeFence()
    {
        return m_NativeFence.get();
    }
} // namespace FE::Osmium
