#include <FeGPU/CommandBuffer/VKCommandBuffer.h>
#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Image/VKImage.h>

namespace FE::GPU
{
    VKCommandBuffer::VKCommandBuffer(VKDevice& dev, CommandQueueClass cmdQueueClass)
        : m_Device(&dev)
        , m_IsUpdating(false)
    {
        auto& nativeDevice = m_Device->GetNativeDevice();
        vk::CommandBufferAllocateInfo allocateInfo{};
        allocateInfo.commandPool        = m_Device->GetCommandPool(cmdQueueClass);
        allocateInfo.commandBufferCount = 1;
        allocateInfo.level              = vk::CommandBufferLevel::ePrimary;

        using Allocator = StdHeapAllocator<vk::UniqueCommandBuffer>;
        auto buffers    = nativeDevice.allocateCommandBuffersUnique<VULKAN_HPP_DEFAULT_DISPATCHER_TYPE, Allocator>(allocateInfo);
        m_CommandBuffer = std::move(buffers.front());
    }

    VKCommandBuffer::VKCommandBuffer(VKDevice& dev, UInt32 queueFamilyIndex)
        : m_Device(&dev)
        , m_IsUpdating(false)
    {
        auto& nativeDevice = m_Device->GetNativeDevice();
        vk::CommandBufferAllocateInfo allocateInfo{};
        allocateInfo.commandPool        = m_Device->GetCommandPool(queueFamilyIndex);
        allocateInfo.commandBufferCount = 1;
        allocateInfo.level              = vk::CommandBufferLevel::ePrimary;

        using Allocator = StdHeapAllocator<vk::UniqueCommandBuffer>;
        auto buffers    = nativeDevice.allocateCommandBuffersUnique<VULKAN_HPP_DEFAULT_DISPATCHER_TYPE, Allocator>(allocateInfo);
        m_CommandBuffer = std::move(buffers.front());
    }

    vk::CommandBuffer& VKCommandBuffer::GetNativeBuffer()
    {
        return m_CommandBuffer.get();
    }

    void VKCommandBuffer::Begin()
    {
        FE_ASSERT(!m_IsUpdating);
        m_IsUpdating = true;
        m_CommandBuffer->begin(vk::CommandBufferBeginInfo{});
    }

    void VKCommandBuffer::End()
    {
        FE_ASSERT(m_IsUpdating);
        m_IsUpdating = false;
        m_CommandBuffer->end();
    }

    void VKCommandBuffer::Draw(UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance)
    {
        m_CommandBuffer->draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VKCommandBuffer::DrawIndexed(
        UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance)
    {
        m_CommandBuffer->drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VKCommandBuffer::SetViewport(const Viewport& viewport)
    {
        vk::Viewport vp{};
        vp.x        = viewport.MinX;
        vp.y        = viewport.MinY;
        vp.width    = viewport.Width();
        vp.height   = viewport.Height();
        vp.minDepth = viewport.MinZ;
        vp.maxDepth = viewport.MaxZ;
        m_CommandBuffer->setViewport(0, 1, &vp);
    }

    void VKCommandBuffer::SetScissor(const Scissor& scissor)
    {
        vk::Rect2D rect{};
        rect.offset = vk::Offset2D(scissor.MinX, scissor.MaxX);
        rect.extent = vk::Extent2D(scissor.Width(), scissor.Height());
        m_CommandBuffer->setScissor(0, 1, &rect);
    }

    void VKCommandBuffer::ResourceTransitionBarriers(const Vector<ResourceTransitionBarrierDesc>& barriers)
    {
        Vector<vk::ImageMemoryBarrier> nativeBarriers;
        for (auto& barrier : barriers)
        {
            auto* img = fe_dynamic_cast<VKImage*>(barrier.Image);
            if (img == nullptr)
            {
                continue;
            }

            auto before = VKConvert(barrier.StateBefore);
            auto after  = VKConvert(barrier.StateAfter);
            if (before == after)
            {
                continue;
            }

            vk::ImageMemoryBarrier& imageMemoryBarrier = nativeBarriers.emplace_back();
            imageMemoryBarrier.oldLayout               = before;
            imageMemoryBarrier.newLayout               = after;
            imageMemoryBarrier.srcQueueFamilyIndex     = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex     = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image                   = img->Image;
            imageMemoryBarrier.subresourceRange        = VKConvert(barrier.SubresourceRange);

            imageMemoryBarrier.srcAccessMask = GetAccessMask(barrier.StateBefore);
            imageMemoryBarrier.dstAccessMask = GetAccessMask(barrier.StateAfter);

            if (after == vk::ImageLayout::eShaderReadOnlyOptimal)
            {
                imageMemoryBarrier.srcAccessMask |= vk::AccessFlagBits::eHostWrite;
                imageMemoryBarrier.srcAccessMask |= vk::AccessFlagBits::eTransferWrite;
            }
        }

        if (nativeBarriers.empty())
            return;

        m_CommandBuffer->pipelineBarrier(
            vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlagBits::eByRegion,
            0, nullptr, 0, nullptr, static_cast<UInt32>(nativeBarriers.size()), nativeBarriers.data());
    }

    void VKCommandBuffer::MemoryBarrier()
    {
        vk::MemoryBarrier barrier{};
        barrier.dstAccessMask = barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead;
        m_CommandBuffer->pipelineBarrier(
            vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlagBits::eByRegion,
            1, &barrier, 0, nullptr, 0, nullptr);
    }
} // namespace FE::GPU
