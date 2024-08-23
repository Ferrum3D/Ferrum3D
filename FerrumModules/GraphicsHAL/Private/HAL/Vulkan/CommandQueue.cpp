#include <FeCore/Containers/SmallVector.h>
#include <HAL/Vulkan/CommandList.h>
#include <HAL/Vulkan/CommandQueue.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/Fence.h>

namespace FE::Graphics::Vulkan
{
    CommandQueue::CommandQueue(HAL::Device* pDevice, const CommandQueueDesc& desc)
        : m_Desc(desc)
    {
        m_pDevice = pDevice;
        vkGetDeviceQueue(NativeCast(m_pDevice), m_Desc.QueueFamilyIndex, m_Desc.QueueIndex, &m_Queue);
    }


    const CommandQueueDesc& CommandQueue::GetDesc() const
    {
        return m_Desc;
    }


    void CommandQueue::SignalFence(HAL::Fence* fence)
    {
        SubmitBuffers({}, fence, HAL::SubmitFlags::None);
    }


    void CommandQueue::SubmitBuffers(festd::span<HAL::CommandList*> commandLists, HAL::Fence* signalFence, HAL::SubmitFlags flags)
    {
        festd::small_vector<VkCommandBuffer> commandBuffers;
        commandBuffers.reserve(commandLists.size());
        for (HAL::CommandList* pCommandList : commandLists)
        {
            commandBuffers.push_back(NativeCast(pCommandList));
        }

        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.pCommandBuffers = commandBuffers.data();
        info.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if ((flags & HAL::SubmitFlags::FrameBegin) != HAL::SubmitFlags::None)
        {
            info.waitSemaphoreCount = ImplCast(m_pDevice)->GetWaitSemaphores(&info.pWaitSemaphores);
        }
        if ((flags & HAL::SubmitFlags::FrameEnd) != HAL::SubmitFlags::None)
        {
            info.signalSemaphoreCount = ImplCast(m_pDevice)->GetSignalSemaphores(&info.pSignalSemaphores);
        }

        festd::small_vector<VkPipelineStageFlags> waitDstFlags(info.waitSemaphoreCount, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        info.pWaitDstStageMask = waitDstFlags.data();

        const VkFence vkFence = signalFence ? NativeCast(signalFence) : VK_NULL_HANDLE;
        vkQueueSubmit(m_Queue, 1, &info, vkFence);
    }
} // namespace FE::Graphics::Vulkan
