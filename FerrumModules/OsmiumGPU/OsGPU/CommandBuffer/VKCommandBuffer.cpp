#include <OsGPU/Buffer/VKBuffer.h>
#include <OsGPU/CommandBuffer/VKCommandBuffer.h>
#include <OsGPU/Common/VKBaseTypes.h>
#include <OsGPU/Common/VKViewport.h>
#include <OsGPU/Descriptors/VKDescriptorTable.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Framebuffer/VKFramebuffer.h>
#include <OsGPU/Image/VKImage.h>
#include <OsGPU/Image/VKImageSubresource.h>
#include <OsGPU/ImageView/IImageView.h>
#include <OsGPU/Pipeline/VKGraphicsPipeline.h>
#include <OsGPU/RenderPass/VKRenderPass.h>
#include <OsGPU/Resource/VKResourceState.h>

namespace FE::Osmium
{
    VKCommandBuffer::VKCommandBuffer(VKDevice& dev, CommandQueueClass cmdQueueClass)
        : m_Device(&dev)
        , m_IsUpdating(false)
    {
        auto nativeDevice = m_Device->GetNativeDevice();
        m_CommandPool     = m_Device->GetCommandPool(cmdQueueClass);
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool        = m_CommandPool;
        allocateInfo.commandBufferCount = 1;
        allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(nativeDevice, &allocateInfo, &m_CommandBuffer);
    }

    VKCommandBuffer::VKCommandBuffer(VKDevice& dev, UInt32 queueFamilyIndex)
        : m_Device(&dev)
        , m_IsUpdating(false)
    {
        auto nativeDevice = m_Device->GetNativeDevice();
        m_CommandPool     = m_Device->GetCommandPool(queueFamilyIndex);
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.commandPool        = m_CommandPool;
        allocateInfo.commandBufferCount = 1;
        allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(nativeDevice, &allocateInfo, &m_CommandBuffer);
    }

    VkCommandBuffer VKCommandBuffer::GetNativeBuffer()
    {
        return m_CommandBuffer;
    }

    void VKCommandBuffer::Begin()
    {
        FE_ASSERT(!m_IsUpdating);
        m_IsUpdating = true;
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
    }

    void VKCommandBuffer::End()
    {
        FE_ASSERT(m_IsUpdating);
        m_IsUpdating = false;
        vkEndCommandBuffer(m_CommandBuffer);
    }

    void VKCommandBuffer::Draw(UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance)
    {
        vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VKCommandBuffer::DrawIndexed(
        UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance)
    {
        vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void VKCommandBuffer::SetViewport(const Viewport& viewport)
    {
        auto vp = VKConvert(viewport);
        vkCmdSetViewport(m_CommandBuffer, 0, 1, &vp);
    }

    void VKCommandBuffer::SetScissor(const Scissor& scissor)
    {
        auto rect = VKConvert(scissor);
        vkCmdSetScissor(m_CommandBuffer, 0, 1, &rect);
    }

    void VKCommandBuffer::ResourceTransitionBarriers(const List<ResourceTransitionBarrierDesc>& barriers)
    {
        List<VkImageMemoryBarrier> nativeBarriers;
        for (auto& barrier : barriers)
        {
            auto* img = fe_dynamic_cast<VKImage*>(barrier.Image);
            if (img == nullptr)
            {
                continue;
            }

            auto stateBefore =
                barrier.Image->GetState(barrier.SubresourceRange.MinArraySlice, barrier.SubresourceRange.MinMipSlice);
            auto before = VKConvert(stateBefore);
            auto after  = VKConvert(barrier.StateAfter);
            if (before == after)
            {
                continue;
            }

            auto& imageMemoryBarrier               = nativeBarriers.Emplace();
            imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout           = before;
            imageMemoryBarrier.newLayout           = after;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image               = img->Image;
            imageMemoryBarrier.subresourceRange    = VKConvert(barrier.SubresourceRange);

            imageMemoryBarrier.srcAccessMask = GetAccessMask(stateBefore);
            imageMemoryBarrier.dstAccessMask = GetAccessMask(barrier.StateAfter);

            if (after == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                imageMemoryBarrier.srcAccessMask |= VK_ACCESS_HOST_WRITE_BIT;
                imageMemoryBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_WRITE_BIT;
            }

            barrier.Image->SetState(barrier.SubresourceRange, barrier.StateAfter);
        }

        if (nativeBarriers.Empty())
        {
            return;
        }

        vkCmdPipelineBarrier(
            m_CommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT,
            0, nullptr, 0, nullptr, static_cast<UInt32>(nativeBarriers.Size()), nativeBarriers.Data());
    }

    void VKCommandBuffer::MemoryBarrier()
    {
        VkMemoryBarrier nativeBarrier{};
        nativeBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        nativeBarrier.dstAccessMask = nativeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(
            m_CommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT,
            1, &nativeBarrier, 0, nullptr, 0, nullptr);
    }

    void VKCommandBuffer::BindDescriptorTables(const List<IDescriptorTable*>& descriptorTables, IGraphicsPipeline* pipeline)
    {
        List<VkDescriptorSet> nativeSets;
        for (auto& table : descriptorTables)
        {
            nativeSets.Push(fe_assert_cast<VKDescriptorTable*>(table)->GetNativeSet());
        }

        auto* vkPipeline = fe_assert_cast<VKGraphicsPipeline*>(pipeline);
        vkCmdBindDescriptorSets(
            m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline->GetNativeLayout(), 0, static_cast<UInt32>(nativeSets.Size()),
            nativeSets.Data(), 0, nullptr);
    }

    void VKCommandBuffer::BeginRenderPass(
        IRenderPass* renderPass, IFramebuffer* framebuffer, const List<ClearValueDesc>& clearValues)
    {
        Vector<VkClearValue> vkClearValues{};
        for (const auto& clearValue : clearValues)
        {
            auto& vkClearValue = vkClearValues.emplace_back();
            if (clearValue.IsDepth)
            {
                vkClearValue.depthStencil.depth   = clearValue.DepthValue;
                vkClearValue.depthStencil.stencil = clearValue.StencilValue;
            }
            else
            {
                vkClearValue.color.float32[0] = clearValue.ColorValue.R32();
                vkClearValue.color.float32[1] = clearValue.ColorValue.G32();
                vkClearValue.color.float32[2] = clearValue.ColorValue.B32();
                vkClearValue.color.float32[3] = clearValue.ColorValue.A32();
            }
        }

        VkRenderPassBeginInfo info{};
        info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.framebuffer       = fe_assert_cast<VKFramebuffer*>(framebuffer)->GetNativeFramebuffer();
        info.renderPass        = fe_assert_cast<VKRenderPass*>(renderPass)->GetNativeRenderPass();
        info.clearValueCount   = static_cast<UInt32>(vkClearValues.size());
        info.pClearValues      = vkClearValues.data();
        info.renderArea.offset = VkOffset2D{ 0, 0 };
        info.renderArea.extent = VkExtent2D{ framebuffer->GetDesc().Width, framebuffer->GetDesc().Height };
        vkCmdBeginRenderPass(m_CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VKCommandBuffer::EndRenderPass()
    {
        vkCmdEndRenderPass(m_CommandBuffer);
    }

    void VKCommandBuffer::BindGraphicsPipeline(IGraphicsPipeline* pipeline)
    {
        auto nativePipeline = fe_assert_cast<VKGraphicsPipeline*>(pipeline)->GetNativePipeline();
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, nativePipeline);
    }

    void VKCommandBuffer::BindVertexBuffer(UInt32 slot, IBuffer* buffer)
    {
        auto nativeBuffer   = fe_assert_cast<VKBuffer*>(buffer)->Buffer;
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(m_CommandBuffer, slot, 1, &nativeBuffer, &offset);
    }

    void VKCommandBuffer::BindIndexBuffer(IBuffer* buffer)
    {
        auto nativeBuffer = fe_assert_cast<VKBuffer*>(buffer)->Buffer;
        vkCmdBindIndexBuffer(m_CommandBuffer, nativeBuffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void VKCommandBuffer::CopyBuffers(IBuffer* source, IBuffer* dest, const BufferCopyRegion& region)
    {
        auto nativeSrc = fe_assert_cast<VKBuffer*>(source)->Buffer;
        auto nativeDst = fe_assert_cast<VKBuffer*>(dest)->Buffer;

        VkBufferCopy copy{};
        copy.size      = region.Size;
        copy.dstOffset = region.DestOffset;
        copy.srcOffset = region.SourceOffset;
        vkCmdCopyBuffer(m_CommandBuffer, nativeSrc, nativeDst, 1, &copy);
    }

    void VKCommandBuffer::CopyBufferToImage(IBuffer* source, IImage* dest, const BufferImageCopyRegion& region)
    {
        auto nativeSrc = fe_assert_cast<VKBuffer*>(source)->Buffer;
        auto nativeDst = fe_assert_cast<VKImage*>(dest)->Image;

        VkBufferImageCopy copy{};
        copy.bufferOffset      = region.BufferOffset;
        copy.bufferRowLength   = 0;
        copy.bufferImageHeight = 0;

        auto subresource                     = VKConvert(region.ImageSubresource);
        copy.imageSubresource.aspectMask     = subresource.aspectMask;
        copy.imageSubresource.mipLevel       = subresource.mipLevel;
        copy.imageSubresource.baseArrayLayer = subresource.arrayLayer;
        copy.imageSubresource.layerCount     = 1;

        copy.imageOffset = VKConvert(region.ImageOffset);
        copy.imageExtent = VKConvert(region.ImageSize);

        vkCmdCopyBufferToImage(m_CommandBuffer, nativeSrc, nativeDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
    }

    void VKCommandBuffer::BlitImage(IImage* source, IImage* dest, const ImageBlitRegion& region)
    {
        auto nativeSrc = fe_assert_cast<VKImage*>(source)->Image;
        auto nativeDst = fe_assert_cast<VKImage*>(dest)->Image;

        auto srcSubresource = VKConvert(region.Source);
        auto dstSubresource = VKConvert(region.Dest);

        VkImageBlit nativeBlit{};
        // SRC
        nativeBlit.srcSubresource.aspectMask     = srcSubresource.aspectMask;
        nativeBlit.srcSubresource.baseArrayLayer = srcSubresource.arrayLayer;
        nativeBlit.srcSubresource.layerCount     = 1;
        nativeBlit.srcSubresource.mipLevel       = srcSubresource.mipLevel;

        nativeBlit.srcOffsets[0] = VKConvert(region.SourceBounds[0]);
        nativeBlit.srcOffsets[1] = VKConvert(region.SourceBounds[1]);

        // DEST
        nativeBlit.dstSubresource.aspectMask     = dstSubresource.aspectMask;
        nativeBlit.dstSubresource.baseArrayLayer = dstSubresource.arrayLayer;
        nativeBlit.dstSubresource.layerCount     = 1;
        nativeBlit.dstSubresource.mipLevel       = dstSubresource.mipLevel;

        nativeBlit.dstOffsets[0] = VKConvert(region.DestBounds[0]);
        nativeBlit.dstOffsets[1] = VKConvert(region.DestBounds[1]);

        vkCmdBlitImage(
            m_CommandBuffer, nativeSrc, VKConvert(source->GetState(region.Source)), nativeDst,
            VKConvert(dest->GetState(region.Dest)), 1, &nativeBlit, VK_FILTER_LINEAR);
    }

    VKCommandBuffer::~VKCommandBuffer()
    {
        vkFreeCommandBuffers(m_Device->GetNativeDevice(), m_CommandPool, 1, &m_CommandBuffer);
    }
} // namespace FE::Osmium
