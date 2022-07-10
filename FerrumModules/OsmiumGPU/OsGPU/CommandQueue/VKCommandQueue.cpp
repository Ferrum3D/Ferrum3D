#include <OsGPU/CommandBuffer/VKCommandBuffer.h>
#include <OsGPU/CommandQueue/VKCommandQueue.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Fence/VKFence.h>

namespace FE::Osmium
{
    VKCommandQueue::VKCommandQueue(VKDevice& dev, const VKCommandQueueDesc& desc)
        : m_Desc(desc)
        , m_Device(&dev)
    {
        vkGetDeviceQueue(m_Device->GetNativeDevice(), m_Desc.QueueFamilyIndex, m_Desc.QueueIndex, &m_Queue);
    }

    const VKCommandQueueDesc& VKCommandQueue::GetDesc() const
    {
        return m_Desc;
    }

    VkQueue VKCommandQueue::GetNativeQueue()
    {
        return m_Queue;
    }

    void VKCommandQueue::SignalFence(IFence* fence)
    {
        SubmitBuffers({}, fence, SubmitFlags::None);
    }

    void VKCommandQueue::SubmitBuffers(const ArraySlice<ICommandBuffer*>& buffers, IFence* signalFence, SubmitFlags flags)
    {
        List<VkCommandBuffer> nativeBuffers;
        nativeBuffers.Reserve(buffers.Length());
        for (auto& buf : buffers)
        {
            auto* vkBuffer = fe_assert_cast<VKCommandBuffer*>(buf);
            nativeBuffers.Push(vkBuffer->GetNativeBuffer());
        }

        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.pCommandBuffers    = nativeBuffers.Data();
        info.commandBufferCount = static_cast<UInt32>(nativeBuffers.Size());

        if ((flags & SubmitFlags::FrameBegin) != SubmitFlags::None)
        {
            info.waitSemaphoreCount = m_Device->GetWaitSemaphores(&info.pWaitSemaphores);
        }
        if ((flags & SubmitFlags::FrameEnd) != SubmitFlags::None)
        {
            info.signalSemaphoreCount = m_Device->GetSignalSemaphores(&info.pSignalSemaphores);
        }

        List<VkPipelineStageFlags> waitDstFlags(info.waitSemaphoreCount, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        info.pWaitDstStageMask = waitDstFlags.Data();

        VkFence vkFence = signalFence ? fe_assert_cast<VKFence*>(signalFence)->GetNativeFence() : VK_NULL_HANDLE;
        vkQueueSubmit(m_Queue, 1, &info, vkFence);
    }
} // namespace FE::Osmium
