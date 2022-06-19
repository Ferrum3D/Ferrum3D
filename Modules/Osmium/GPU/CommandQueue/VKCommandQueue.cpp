#include <GPU/CommandBuffer/VKCommandBuffer.h>
#include <GPU/CommandQueue/VKCommandQueue.h>
#include <GPU/Device/VKDevice.h>
#include <GPU/Fence/VKFence.h>

namespace FE::GPU
{
    VKCommandQueue::VKCommandQueue(VKDevice& dev, const VKCommandQueueDesc& desc)
        : m_Desc(desc)
        , m_Device(&dev)
    {
        m_Queue = m_Device->GetNativeDevice().getQueue(m_Desc.QueueFamilyIndex, m_Desc.QueueIndex);
    }

    const VKCommandQueueDesc& VKCommandQueue::GetDesc() const
    {
        return m_Desc;
    }

    vk::Queue VKCommandQueue::GetNativeQueue()
    {
        return m_Queue;
    }

    void VKCommandQueue::SignalFence(const Shared<IFence>& fence)
    {
        SubmitBuffers({}, fence, SubmitFlags::None);
    }

    void VKCommandQueue::SubmitBuffers(
        const List<ICommandBuffer*>& buffers, const Shared<IFence>& signalFence, SubmitFlags flags)
    {
        Vector<vk::CommandBuffer> nativeBuffers{};
        nativeBuffers.reserve(buffers.Size());
        for (auto& buf : buffers)
        {
            auto* vkBuffer = fe_assert_cast<VKCommandBuffer*>(buf);
            nativeBuffers.push_back(vkBuffer->GetNativeBuffer());
        }

        vk::SubmitInfo info{};
        info.pCommandBuffers    = nativeBuffers.data();
        info.commandBufferCount = static_cast<UInt32>(nativeBuffers.size());

        if ((flags & SubmitFlags::FrameBegin) != SubmitFlags::None)
        {
            info.waitSemaphoreCount = m_Device->GetWaitSemaphores(&info.pWaitSemaphores);
        }
        if ((flags & SubmitFlags::FrameEnd) != SubmitFlags::None)
        {
            info.signalSemaphoreCount = m_Device->GetSignalSemaphores(&info.pSignalSemaphores);
        }

        Vector<vk::PipelineStageFlags> waitDstFlags(info.waitSemaphoreCount, vk::PipelineStageFlagBits::eAllCommands);
        info.pWaitDstStageMask = waitDstFlags.data();

        vk::Fence vkFence = signalFence ? fe_assert_cast<VKFence*>(signalFence.GetRaw())->GetNativeFence() : nullptr;
        m_Queue.submit({ info }, vkFence);
    }
} // namespace FE::GPU
