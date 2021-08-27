#include <FeGPU/CommandBuffer/VKCommandBuffer.h>
#include <FeGPU/CommandQueue/VKCommandQueue.h>
#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Fence/VKFence.h>
#include <FeGPU/SwapChain/VKSwapChain.h>

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

    void VKCommandQueue::SignalFence(const RefCountPtr<IFence>& fence)
    {
        SubmitBuffers({}, fence);
    }

    void VKCommandQueue::SubmitBuffers(const Vector<RefCountPtr<ICommandBuffer>>& buffers, const RefCountPtr<IFence>& signalFence)
    {
        Vector<vk::CommandBuffer> nativeBuffers{};
        nativeBuffers.reserve(buffers.size());
        for (auto& buf : buffers)
        {
            auto* vkBuffer = fe_assert_cast<VKCommandBuffer*>(buf.GetRaw());
            nativeBuffers.push_back(vkBuffer->GetNativeBuffer());
        }

        vk::SubmitInfo info{};
        info.pCommandBuffers      = nativeBuffers.data();
        info.commandBufferCount   = static_cast<UInt32>(nativeBuffers.size());
        info.pWaitSemaphores      = &VKSwapChain::m_ImageAvailableSemaphore.get();
        info.waitSemaphoreCount   = 1;
        info.pSignalSemaphores    = &VKSwapChain::m_RenderFinishedSemaphore.get();
        info.signalSemaphoreCount = 1;

        vk::PipelineStageFlags waitDstFlags = vk::PipelineStageFlagBits::eAllCommands;
        info.pWaitDstStageMask              = &waitDstFlags;

        vk::Fence vkFence = signalFence ? fe_assert_cast<VKFence*>(signalFence.GetRaw())->GetNativeFence() : nullptr;
        m_Queue.submit({ info }, vkFence);
    }
} // namespace FE::GPU
