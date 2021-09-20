#include <GPU/Buffer/VKBuffer.h>
#include <GPU/CommandBuffer/VKCommandBuffer.h>
#include <GPU/Descriptors/VKDescriptorTable.h>
#include <GPU/Device/VKDevice.h>
#include <GPU/Framebuffer/VKFramebuffer.h>
#include <GPU/Image/VKImage.h>
#include <GPU/Pipeline/VKGraphicsPipeline.h>
#include <GPU/RenderPass/VKRenderPass.h>

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
        vk::Viewport vp = VKConvert(viewport);
        m_CommandBuffer->setViewport(0, 1, &vp);
    }

    void VKCommandBuffer::SetScissor(const Scissor& scissor)
    {
        vk::Rect2D rect = VKConvert(scissor);
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
        {
            return;
        }

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

    void VKCommandBuffer::BindDescriptorTables(const Vector<IDescriptorTable*>& descriptorTables, IGraphicsPipeline* pipeline)
    {
        Vector<vk::DescriptorSet> nativeSets;
        for (auto& table : descriptorTables)
        {
            nativeSets.push_back(fe_assert_cast<VKDescriptorTable*>(table)->GetNativeSet());
        }

        auto* vkPipeline = fe_assert_cast<VKGraphicsPipeline*>(pipeline);
        m_CommandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vkPipeline->GetNativeLayout(), 0, nativeSets, {});
    }

    void VKCommandBuffer::BeginRenderPass(IRenderPass* renderPass, IFramebuffer* framebuffer, const ClearValueDesc& clearValue)
    {
        vk::ClearValue vkClearValue{};
        vkClearValue.color.float32[0] = clearValue.ColorValue.R32();
        vkClearValue.color.float32[1] = clearValue.ColorValue.B32();
        vkClearValue.color.float32[2] = clearValue.ColorValue.B32();
        vkClearValue.color.float32[3] = clearValue.ColorValue.A32();

        vk::RenderPassBeginInfo info{};
        info.framebuffer       = fe_assert_cast<VKFramebuffer*>(framebuffer)->GetNativeFramebuffer();
        info.renderPass        = fe_assert_cast<VKRenderPass*>(renderPass)->GetNativeRenderPass();
        info.clearValueCount   = 1;
        info.pClearValues      = &vkClearValue;
        info.renderArea.offset = vk::Offset2D{ 0, 0 };
        info.renderArea.extent = vk::Extent2D{ framebuffer->GetDesc().Width, framebuffer->GetDesc().Height };
        m_CommandBuffer->beginRenderPass(info, vk::SubpassContents::eInline);
    }

    void VKCommandBuffer::EndRenderPass()
    {
        m_CommandBuffer->endRenderPass();
    }

    void VKCommandBuffer::BindGraphicsPipeline(IGraphicsPipeline* pipeline)
    {
        auto& nativePipeline = fe_assert_cast<VKGraphicsPipeline*>(pipeline)->GetNativePipeline();
        m_CommandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, nativePipeline);
    }

    void VKCommandBuffer::BindVertexBuffer(UInt32 slot, IBuffer* buffer)
    {
        auto nativeBuffer     = fe_assert_cast<VKBuffer*>(buffer)->Buffer.get();
        vk::DeviceSize offset = 0;
        m_CommandBuffer->bindVertexBuffers(slot, { nativeBuffer }, { offset });
    }

    void VKCommandBuffer::BindIndexBuffer(IBuffer* buffer)
    {
        auto nativeBuffer = fe_assert_cast<VKBuffer*>(buffer)->Buffer.get();
        m_CommandBuffer->bindIndexBuffer(nativeBuffer, 0, vk::IndexType::eUint32);
    }

    void VKCommandBuffer::CopyBuffers(IBuffer* source, IBuffer* dest, const BufferCopyRegion& region)
    {
        auto nativeSrc = fe_assert_cast<VKBuffer*>(source)->Buffer.get();
        auto nativeDst = fe_assert_cast<VKBuffer*>(dest)->Buffer.get();
        
        vk::BufferCopy copy{};
        copy.size      = region.Size;
        copy.dstOffset = region.DestOffset;
        copy.srcOffset = region.SourceOffset;
        m_CommandBuffer->copyBuffer(nativeSrc, nativeDst, { copy });
    }
} // namespace FE::GPU
