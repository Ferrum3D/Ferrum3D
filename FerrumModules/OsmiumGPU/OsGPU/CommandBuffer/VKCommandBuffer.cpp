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
        m_CommandPool = m_Device->GetCommandPool(cmdQueueClass);
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = m_CommandPool;
        allocateInfo.commandBufferCount = 1;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(nativeDevice, &allocateInfo, &m_CommandBuffer);
    }

    VKCommandBuffer::VKCommandBuffer(VKDevice& dev, uint32_t queueFamilyIndex)
        : m_Device(&dev)
        , m_IsUpdating(false)
    {
        auto nativeDevice = m_Device->GetNativeDevice();
        m_CommandPool = m_Device->GetCommandPool(queueFamilyIndex);
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.commandPool = m_CommandPool;
        allocateInfo.commandBufferCount = 1;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

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

    void VKCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VKCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                      uint32_t firstInstance)
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

    void VKCommandBuffer::ResourceTransitionBarriers(const ArraySlice<ImageBarrierDesc>& imageBarriers,
                                                     const ArraySlice<BufferBarrierDesc>& bufferBarriers)
    {
        eastl::vector<VkImageMemoryBarrier> nativeImageBarriers;
        eastl::vector<VkBufferMemoryBarrier> nativeBufferBarriers;
        for (auto& barrier : imageBarriers)
        {
            auto* img = fe_assert_cast<VKImage*>(barrier.Image);
            if (img == nullptr)
            {
                continue;
            }

            auto stateBefore = barrier.StateBefore == ResourceState::Automatic
                ? barrier.Image->GetState(barrier.SubresourceRange.MinArraySlice, barrier.SubresourceRange.MinMipSlice)
                : barrier.StateBefore;

            auto before = VKConvert(stateBefore);
            auto after = VKConvert(barrier.StateAfter);
            if (before == after)
            {
                continue;
            }

            auto& imageMemoryBarrier = nativeImageBarriers.push_back();
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = before;
            imageMemoryBarrier.newLayout = after;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = img->Image;
            imageMemoryBarrier.subresourceRange = VKConvert(barrier.SubresourceRange);

            imageMemoryBarrier.srcAccessMask = GetAccessMask(stateBefore);
            imageMemoryBarrier.dstAccessMask = GetAccessMask(barrier.StateAfter);

            if (after == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                imageMemoryBarrier.srcAccessMask |= VK_ACCESS_HOST_WRITE_BIT;
                imageMemoryBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_WRITE_BIT;
            }

            barrier.Image->SetState(barrier.SubresourceRange, barrier.StateAfter);
        }

        for (auto& barrier : bufferBarriers)
        {
            auto* buffer = fe_assert_cast<VKBuffer*>(barrier.Buffer);
            if (buffer == nullptr)
            {
                continue;
            }

            FE_ASSERT_MSG(barrier.StateBefore != ResourceState::Automatic,
                          "Automatic resource state management is currently "
                          "not supported for buffers");

            if (barrier.StateBefore == barrier.StateAfter)
            {
                continue;
            }

            auto& bufferMemoryBarrier = nativeBufferBarriers.push_back();
            bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.buffer = buffer->Buffer;
            bufferMemoryBarrier.offset = barrier.Offset;
            bufferMemoryBarrier.size = barrier.Size;

            bufferMemoryBarrier.srcAccessMask = GetAccessMask(barrier.StateBefore);
            bufferMemoryBarrier.dstAccessMask = GetAccessMask(barrier.StateAfter);
        }

        if (nativeImageBarriers.empty())
        {
            return;
        }

        vkCmdPipelineBarrier(m_CommandBuffer,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT,
                             0,
                             nullptr,
                             static_cast<uint32_t>(nativeBufferBarriers.size()),
                             nativeBufferBarriers.data(),
                             static_cast<uint32_t>(nativeImageBarriers.size()),
                             nativeImageBarriers.data());
    }

    void VKCommandBuffer::MemoryBarrier()
    {
        VkMemoryBarrier nativeBarrier{};
        nativeBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        nativeBarrier.dstAccessMask = nativeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(m_CommandBuffer,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT,
                             1,
                             &nativeBarrier,
                             0,
                             nullptr,
                             0,
                             nullptr);
    }

    void VKCommandBuffer::BindDescriptorTables(const ArraySlice<IDescriptorTable*>& descriptorTables, IGraphicsPipeline* pipeline)
    {
        eastl::vector<VkDescriptorSet> nativeSets;
        for (auto& table : descriptorTables)
        {
            nativeSets.push_back(fe_assert_cast<VKDescriptorTable*>(table)->GetNativeSet());
        }

        auto* vkPipeline = fe_assert_cast<VKGraphicsPipeline*>(pipeline);
        vkCmdBindDescriptorSets(m_CommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                vkPipeline->GetNativeLayout(),
                                0,
                                static_cast<uint32_t>(nativeSets.size()),
                                nativeSets.data(),
                                0,
                                nullptr);
    }

    void VKCommandBuffer::BeginRenderPass(IRenderPass* renderPass, IFramebuffer* framebuffer,
                                          const ArraySlice<ClearValueDesc>& clearValues)
    {
        eastl::vector<VkClearValue> vkClearValues{};
        for (const auto& clearValue : clearValues)
        {
            auto& vkClearValue = vkClearValues.push_back();
            if (clearValue.IsDepth)
            {
                vkClearValue.depthStencil.depth = clearValue.DepthValue;
                vkClearValue.depthStencil.stencil = clearValue.StencilValue;
            }
            else
            {
                for (uint32_t i = 0; i < clearValue.ColorValue.size(); ++i)
                {
                    vkClearValue.color.float32[i] = clearValue.ColorValue[i];
                }
            }
        }

        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.framebuffer = fe_assert_cast<VKFramebuffer*>(framebuffer)->GetNativeFramebuffer();
        info.renderPass = fe_assert_cast<VKRenderPass*>(renderPass)->GetNativeRenderPass();
        info.clearValueCount = static_cast<uint32_t>(vkClearValues.size());
        info.pClearValues = vkClearValues.data();
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

    void VKCommandBuffer::BindIndexBuffer(IBuffer* buffer, uint64_t byteOffset)
    {
        auto nativeBuffer = fe_assert_cast<VKBuffer*>(buffer)->Buffer;
        vkCmdBindIndexBuffer(m_CommandBuffer, nativeBuffer, byteOffset, VK_INDEX_TYPE_UINT32);
    }

    void VKCommandBuffer::BindVertexBuffer(uint32_t slot, IBuffer* buffer, uint64_t byteOffset)
    {
        auto nativeBuffer = fe_assert_cast<VKBuffer*>(buffer)->Buffer;
        vkCmdBindVertexBuffers(m_CommandBuffer, slot, 1, &nativeBuffer, &byteOffset);
    }

    void VKCommandBuffer::BindVertexBuffers(uint32_t startSlot, const ArraySlice<IBuffer*>& buffers,
                                            const ArraySlice<uint64_t>& offsets)
    {
        FE_ASSERT_MSG(buffers.Length() == offsets.Length(), "Number of buffers must be the same as number of offsets");

        eastl::vector<VkBuffer> nativeBuffers(buffers.Length(), VK_NULL_HANDLE);
        for (uint32_t i = 0; i < buffers.Length(); ++i)
        {
            nativeBuffers[i] = fe_assert_cast<VKBuffer*>(buffers[i])->Buffer;
        }

        vkCmdBindVertexBuffers(
            m_CommandBuffer, startSlot, static_cast<uint32_t>(nativeBuffers.size()), nativeBuffers.data(), offsets.Data());
    }

    void VKCommandBuffer::CopyBuffers(IBuffer* source, IBuffer* dest, const BufferCopyRegion& region)
    {
        auto nativeSrc = fe_assert_cast<VKBuffer*>(source)->Buffer;
        auto nativeDst = fe_assert_cast<VKBuffer*>(dest)->Buffer;

        VkBufferCopy copy{};
        copy.size = region.Size;
        copy.dstOffset = region.DestOffset;
        copy.srcOffset = region.SourceOffset;
        vkCmdCopyBuffer(m_CommandBuffer, nativeSrc, nativeDst, 1, &copy);
    }

    void VKCommandBuffer::CopyBufferToImage(IBuffer* source, IImage* dest, const BufferImageCopyRegion& region)
    {
        auto nativeSrc = fe_assert_cast<VKBuffer*>(source)->Buffer;
        auto nativeDst = fe_assert_cast<VKImage*>(dest)->Image;

        VkBufferImageCopy copy{};
        copy.bufferOffset = region.BufferOffset;
        copy.bufferRowLength = 0;
        copy.bufferImageHeight = 0;

        auto subresource = VKConvert(region.ImageSubresource);
        copy.imageSubresource.aspectMask = subresource.aspectMask;
        copy.imageSubresource.mipLevel = subresource.mipLevel;
        copy.imageSubresource.baseArrayLayer = subresource.arrayLayer;
        copy.imageSubresource.layerCount = 1;

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
        nativeBlit.srcSubresource.aspectMask = srcSubresource.aspectMask;
        nativeBlit.srcSubresource.baseArrayLayer = srcSubresource.arrayLayer;
        nativeBlit.srcSubresource.layerCount = 1;
        nativeBlit.srcSubresource.mipLevel = srcSubresource.mipLevel;

        nativeBlit.srcOffsets[0] = VKConvert(region.SourceBounds[0]);
        nativeBlit.srcOffsets[1] = VKConvert(region.SourceBounds[1]);

        // DEST
        nativeBlit.dstSubresource.aspectMask = dstSubresource.aspectMask;
        nativeBlit.dstSubresource.baseArrayLayer = dstSubresource.arrayLayer;
        nativeBlit.dstSubresource.layerCount = 1;
        nativeBlit.dstSubresource.mipLevel = dstSubresource.mipLevel;

        nativeBlit.dstOffsets[0] = VKConvert(region.DestBounds[0]);
        nativeBlit.dstOffsets[1] = VKConvert(region.DestBounds[1]);

        vkCmdBlitImage(m_CommandBuffer,
                       nativeSrc,
                       VKConvert(source->GetState(region.Source)),
                       nativeDst,
                       VKConvert(dest->GetState(region.Dest)),
                       1,
                       &nativeBlit,
                       VK_FILTER_LINEAR);
    }

    class VKCommandBufferDeleter final : public IVKObjectDeleter
    {
        VkCommandBuffer m_Buffer;
        VkCommandPool m_Pool;

    public:
        inline VKCommandBufferDeleter(VkCommandBuffer buffer, VkCommandPool pool)
            : m_Buffer(buffer)
            , m_Pool(pool)
        {
        }

        void Delete(VKDevice* device) override;
    };

    void VKCommandBufferDeleter::Delete(VKDevice* device)
    {
        vkFreeCommandBuffers(device->GetNativeDevice(), m_Pool, 1, &m_Buffer);
    }

    VKCommandBuffer::~VKCommandBuffer()
    {
        m_Device->QueueObjectDelete<VKCommandBufferDeleter>(m_CommandBuffer, m_CommandPool);
    }
} // namespace FE::Osmium
