#include <FeGPU/CommandQueue/VKCommandQueue.h>
#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Fence/VKFence.h>
#include <FeGPU/CommandBuffer/VKCommandBuffer.h>

namespace FE::GPU
{
    VKCommandQueue::VKCommandQueue(VKDevice& dev, const VKCommandQueueDesc& desc)
        : m_Desc(desc)
        , m_Device(&dev)
        , m_Queue(m_Device->GetNativeDevice().getQueue(m_Desc.QueueFamilyIndex, m_Desc.QueueIndex))
    {
    }

    void VKCommandQueue::WaitForFence(const RefCountPtr<IFence>& fence, UInt64 value)
    {
        auto& nativeFence = *static_cast<VKFence*>(fence.GetRaw());
        vk::TimelineSemaphoreSubmitInfo timelineInfo{};
        timelineInfo.waitSemaphoreValueCount = 1;
        timelineInfo.pWaitSemaphoreValues    = &value;

        vk::SubmitInfo submitInfo{};
        submitInfo.pNext              = &timelineInfo;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores    = &nativeFence.GetNativeSemaphore();
        submitInfo.pWaitDstStageMask  = &nativeFence.Flags;

        m_Queue.submit(1, &submitInfo, VK_NULL_HANDLE);
    }

    void VKCommandQueue::SignalFence(const RefCountPtr<IFence>& fence, UInt64 value)
    {
        auto& nativeFence = static_cast<VKFence*>(fence.GetRaw())->GetNativeSemaphore();
        vk::TimelineSemaphoreSubmitInfo timelineInfo{};
        timelineInfo.signalSemaphoreValueCount = 1;
        timelineInfo.pSignalSemaphoreValues    = &value;

        vk::SubmitInfo submitInfo{};
        submitInfo.pNext                = &timelineInfo;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &nativeFence;

        m_Queue.submit(1, &submitInfo, VK_NULL_HANDLE);
    }

    void VKCommandQueue::SubmitBuffers(const Vector<RefCountPtr<ICommandBuffer>>& buffers)
    {
        Vector<vk::CommandBuffer> nativeBuffers{};
        nativeBuffers.reserve(buffers.size());
        for (auto& buf : buffers)
        {
            auto* vkbuffer = static_cast<VKCommandBuffer*>(buf.GetRaw());
            nativeBuffers.push_back(vkbuffer->GetNativeBuffer());
        }

        vk::SubmitInfo info{};
        info.pCommandBuffers = nativeBuffers.data();
        info.commandBufferCount = nativeBuffers.size();

        vk::PipelineStageFlags waitDstFlags = vk::PipelineStageFlagBits::eAllCommands;
        info.pWaitDstStageMask = &waitDstFlags;

        m_Queue.submit(1, &info, VK_NULL_HANDLE);
    }

    const VKCommandQueueDesc& VKCommandQueue::GetDesc() const
    {
        return m_Desc;
    }

    vk::Queue VKCommandQueue::GetNativeQueue()
    {
        return m_Queue;
    }
} // namespace FE::GPU
