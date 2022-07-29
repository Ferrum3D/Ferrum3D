#include <OsGPU/CommandQueue/VKCommandQueue.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Fence/VKFence.h>

namespace FE::Osmium
{
    VKFence::VKFence(VKDevice& dev, FenceState initialState)
        : m_Device(&dev)
    {
        VkFenceCreateInfo fenceCI{};
        fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags = initialState == FenceState::Reset ? 0 : VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_Device->GetNativeDevice(), &fenceCI, VK_NULL_HANDLE, &m_NativeFence);
    }

    void VKFence::SignalOnCPU()
    {
        auto queue = m_Device->GetCommandQueue(CommandQueueClass::Graphics);
        queue->SignalFence(this);
    }

    void VKFence::WaitOnCPU()
    {
        vkWaitForFences(m_Device->GetNativeDevice(), 1, &m_NativeFence, false, 10000000000);
    }

    void VKFence::Reset()
    {
        vkResetFences(m_Device->GetNativeDevice(), 1, &m_NativeFence);
    }

    FenceState VKFence::GetState()
    {
        auto status = vkGetFenceStatus(m_Device->GetNativeDevice(), m_NativeFence);
        return status == VK_SUCCESS ? FenceState::Signaled : FenceState::Reset;
    }

    VkFence VKFence::GetNativeFence()
    {
        return m_NativeFence;
    }

    FE_VK_OBJECT_DELETER(Fence);

    VKFence::~VKFence()
    {
        FE_DELETE_VK_OBJECT(Fence, m_NativeFence);
    }
} // namespace FE::Osmium
