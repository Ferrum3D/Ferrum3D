#include <festd/vector.h>
#include <Graphics/RHI/Vulkan/CommandList.h>
#include <Graphics/RHI/Vulkan/CommandQueue.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/Fence.h>

namespace FE::Graphics::Vulkan
{
    CommandQueue::CommandQueue(RHI::Device* device, CommandQueueDesc desc)
        : m_desc(desc)
    {
        m_device = device;
        vkGetDeviceQueue(NativeCast(m_device), m_desc.m_queueFamilyIndex, m_desc.m_queueIndex, &m_queue);
    }


    const CommandQueueDesc& CommandQueue::GetDesc() const
    {
        return m_desc;
    }


    void CommandQueue::SignalFence(const RHI::FenceSyncPoint& fence)
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


    void CommandQueue::WaitFence(const RHI::FenceSyncPoint& fence)
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


    void CommandQueue::Execute(festd::span<RHI::CommandList* const> commandLists)
    {
        const VkPipelineStageFlags kWaitDstFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        festd::small_vector<VkCommandBuffer> commandBuffers;
        commandBuffers.reserve(commandLists.size());

        for (RHI::CommandList* pCommandList : commandLists)
        {
            commandBuffers.push_back(NativeCast(pCommandList));
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pCommandBuffers = commandBuffers.data();
        submitInfo.commandBufferCount = commandBuffers.size();
        submitInfo.pWaitDstStageMask = &kWaitDstFlags;

        FE_VK_ASSERT(vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE));
    }
} // namespace FE::Graphics::Vulkan
