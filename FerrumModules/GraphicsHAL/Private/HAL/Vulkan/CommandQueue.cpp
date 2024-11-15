#include <FeCore/Containers/SmallVector.h>
#include <HAL/Vulkan/CommandList.h>
#include <HAL/Vulkan/CommandQueue.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/Fence.h>

namespace FE::Graphics::Vulkan
{
    CommandQueue::CommandQueue(HAL::Device* pDevice, CommandQueueDesc desc)
        : m_desc(desc)
    {
        m_pDevice = pDevice;
        vkGetDeviceQueue(NativeCast(m_pDevice), m_desc.m_queueFamilyIndex, m_desc.m_queueIndex, &m_queue);
    }


    const CommandQueueDesc& CommandQueue::GetDesc() const
    {
        return m_desc;
    }


    void CommandQueue::SignalFence(const HAL::FenceSyncPoint& fence)
    {
        const VkSemaphore vkSemaphore = NativeCast(fence.m_fence.Get());

        VkTimelineSemaphoreSubmitInfo timelineSemaphoreInfo{};
        timelineSemaphoreInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineSemaphoreInfo.pSignalSemaphoreValues = &fence.m_value;
        timelineSemaphoreInfo.signalSemaphoreValueCount = 1;

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = &timelineSemaphoreInfo;
        submitInfo.pSignalSemaphores = &vkSemaphore;
        submitInfo.signalSemaphoreCount = 1;

        FE_VK_ASSERT(vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE));
    }


    void CommandQueue::WaitFence(const HAL::FenceSyncPoint& fence)
    {
        const VkSemaphore vkSemaphore = NativeCast(fence.m_fence.Get());
        const VkPipelineStageFlags kWaitDstFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        VkTimelineSemaphoreSubmitInfo timelineSemaphoreInfo{};
        timelineSemaphoreInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineSemaphoreInfo.pWaitSemaphoreValues = &fence.m_value;
        timelineSemaphoreInfo.waitSemaphoreValueCount = 1;

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = &timelineSemaphoreInfo;
        submitInfo.pWaitSemaphores = &vkSemaphore;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitDstStageMask = &kWaitDstFlags;

        FE_VK_ASSERT(vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE));
    }


    void CommandQueue::Execute(festd::span<HAL::CommandList* const> commandLists)
    {
        const VkPipelineStageFlags kWaitDstFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        festd::small_vector<VkCommandBuffer> commandBuffers;
        commandBuffers.reserve(commandLists.size());

        for (HAL::CommandList* pCommandList : commandLists)
        {
            commandBuffers.push_back(NativeCast(pCommandList));
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pCommandBuffers = commandBuffers.data();
        submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        submitInfo.pWaitDstStageMask = &kWaitDstFlags;

        FE_VK_ASSERT(vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE));
    }
} // namespace FE::Graphics::Vulkan
