#include <festd/vector.h>
#include <Graphics/RHI/ImageView.h>
#include <Graphics/RHI/Vulkan/Buffer.h>
#include <Graphics/RHI/Vulkan/CommandList.h>
#include <Graphics/RHI/Vulkan/Common/BaseTypes.h>
#include <Graphics/RHI/Vulkan/Common/Viewport.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/Framebuffer.h>
#include <Graphics/RHI/Vulkan/GraphicsPipeline.h>
#include <Graphics/RHI/Vulkan/Image.h>
#include <Graphics/RHI/Vulkan/ImageSubresource.h>
#include <Graphics/RHI/Vulkan/RenderPass.h>
#include <Graphics/RHI/Vulkan/ResourceState.h>
#include <Graphics/RHI/Vulkan/ShaderResourceGroup.h>

namespace FE::Graphics::Vulkan
{
    CommandList::CommandList(RHI::Device* device)
        : m_isUpdating(false)
    {
        m_device = device;
    }


    RHI::ResultCode CommandList::Init(const RHI::CommandListDesc& desc)
    {
        const VkDevice nativeDevice = NativeCast(m_device);
        m_commandPool = ImplCast(m_device)->GetCommandPool(desc.m_queueKind);
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = m_commandPool;
        allocateInfo.commandBufferCount = 1;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        if (vkAllocateCommandBuffers(nativeDevice, &allocateInfo, &m_commandBuffer) != VK_SUCCESS)
            return RHI::ResultCode::kUnknownError;

        return RHI::ResultCode::kSuccess;
    }


    void CommandList::Begin()
    {
        FE_Assert(!m_isUpdating);
        m_isUpdating = true;
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
    }


    void CommandList::End()
    {
        FE_Assert(m_isUpdating);
        m_isUpdating = false;
        vkEndCommandBuffer(m_commandBuffer);
    }


    void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }


    void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                  uint32_t firstInstance)
    {
        vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }


    void CommandList::SetViewport(RHI::Viewport viewport)
    {
        const VkViewport nativeViewport = VKConvert(viewport);
        vkCmdSetViewport(m_commandBuffer, 0, 1, &nativeViewport);
    }


    void CommandList::SetScissor(RHI::Scissor scissor)
    {
        const VkRect2D rect = VKConvert(scissor);
        vkCmdSetScissor(m_commandBuffer, 0, 1, &rect);
    }


    void CommandList::ResourceTransitionBarriers(festd::span<const RHI::ImageBarrierDesc> imageBarriers,
                                                 festd::span<const RHI::BufferBarrierDesc> bufferBarriers)
    {
        festd::small_vector<VkImageMemoryBarrier> nativeImageBarriers;
        for (const RHI::ImageBarrierDesc& barrier : imageBarriers)
        {
            if (barrier.m_image == nullptr)
                continue;

            auto stateBefore = barrier.m_stateBefore == RHI::ResourceState::kAutomatic
                ? barrier.m_image->GetState(barrier.m_subresourceRange.m_minArraySlice, barrier.m_subresourceRange.m_minMipSlice)
                : barrier.m_stateBefore;

            const VkImageLayout before = VKConvert(stateBefore);
            const VkImageLayout after = VKConvert(barrier.m_stateAfter);
            if (before == after)
                continue;

            VkImageMemoryBarrier& imageMemoryBarrier = nativeImageBarriers.emplace_back();
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = before;
            imageMemoryBarrier.newLayout = after;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = NativeCast(barrier.m_image);
            imageMemoryBarrier.subresourceRange = VKConvert(barrier.m_subresourceRange);

            imageMemoryBarrier.srcAccessMask = GetAccessMask(stateBefore);
            imageMemoryBarrier.dstAccessMask = GetAccessMask(barrier.m_stateAfter);

            if (after == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                imageMemoryBarrier.srcAccessMask |= VK_ACCESS_HOST_WRITE_BIT;
                imageMemoryBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_WRITE_BIT;
            }

            barrier.m_image->SetState(barrier.m_subresourceRange, barrier.m_stateAfter);
        }

        festd::small_vector<VkBufferMemoryBarrier> nativeBufferBarriers;
        for (const RHI::BufferBarrierDesc& barrier : bufferBarriers)
        {
            if (barrier.m_buffer == nullptr)
                continue;

            FE_AssertMsg(barrier.m_stateBefore != RHI::ResourceState::kAutomatic,
                         "Automatic resource state management is currently "
                         "not supported for buffers");

            if (barrier.m_stateBefore == barrier.m_stateAfter)
                continue;

            VkBufferMemoryBarrier& bufferMemoryBarrier = nativeBufferBarriers.emplace_back();
            bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.buffer = NativeCast(barrier.m_buffer);
            bufferMemoryBarrier.offset = barrier.m_offset;
            bufferMemoryBarrier.size = barrier.m_size;

            bufferMemoryBarrier.srcAccessMask = GetAccessMask(barrier.m_stateBefore);
            bufferMemoryBarrier.dstAccessMask = GetAccessMask(barrier.m_stateAfter);
        }

        if (nativeImageBarriers.empty())
            return;

        vkCmdPipelineBarrier(m_commandBuffer,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT,
                             0,
                             nullptr,
                             nativeBufferBarriers.size(),
                             nativeBufferBarriers.data(),
                             nativeImageBarriers.size(),
                             nativeImageBarriers.data());
    }


    void CommandList::MemoryBarrier()
    {
        VkMemoryBarrier nativeBarrier{};
        nativeBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        nativeBarrier.dstAccessMask = nativeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(m_commandBuffer,
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


    void CommandList::BindShaderResourceGroups(festd::span<RHI::ShaderResourceGroup* const> shaderResourceGroups,
                                               RHI::GraphicsPipeline* pipeline)
    {
        festd::small_vector<VkDescriptorSet> descriptorSets;
        for (const RHI::ShaderResourceGroup* table : shaderResourceGroups)
        {
            descriptorSets.push_back(ImplCast(table)->GetNativeSet());
        }

        vkCmdBindDescriptorSets(m_commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                ImplCast(pipeline)->GetNativeLayout(),
                                0,
                                descriptorSets.size(),
                                descriptorSets.data(),
                                0,
                                nullptr);
    }


    void CommandList::BeginRenderPass(RHI::RenderPass* renderPass, RHI::Framebuffer* framebuffer,
                                      festd::span<const RHI::ClearValueDesc> clearValues)
    {
        festd::small_vector<VkClearValue> vkClearValues{};
        for (const RHI::ClearValueDesc& clearValue : clearValues)
        {
            VkClearValue& vkClearValue = vkClearValues.emplace_back();
            if (clearValue.m_isDepthStencil)
            {
                vkClearValue.depthStencil.depth = clearValue.m_depthValue;
                vkClearValue.depthStencil.stencil = clearValue.m_stencilValue;
            }
            else
            {
                memcpy(vkClearValue.color.float32, clearValue.m_colorValue.Data(), sizeof(Color4F));
            }
        }

        VkRenderPassBeginInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.framebuffer = NativeCast(framebuffer);
        info.renderPass = NativeCast(renderPass);
        info.clearValueCount = vkClearValues.size();
        info.pClearValues = vkClearValues.data();
        info.renderArea.offset = VkOffset2D{ 0, 0 };
        info.renderArea.extent = VkExtent2D{ framebuffer->GetDesc().m_width, framebuffer->GetDesc().m_height };
        vkCmdBeginRenderPass(m_commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }


    void CommandList::EndRenderPass()
    {
        vkCmdEndRenderPass(m_commandBuffer);
    }


    void CommandList::BindGraphicsPipeline(RHI::GraphicsPipeline* pipeline)
    {
        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, NativeCast(pipeline));
    }


    void CommandList::BindIndexBuffer(RHI::Buffer* buffer, uint64_t byteOffset)
    {
        vkCmdBindIndexBuffer(m_commandBuffer, NativeCast(buffer), byteOffset, VK_INDEX_TYPE_UINT32);
    }


    void CommandList::BindVertexBuffer(uint32_t slot, RHI::Buffer* buffer, uint64_t byteOffset)
    {
        const VkBuffer nativeBuffer = NativeCast(buffer);
        vkCmdBindVertexBuffers(m_commandBuffer, slot, 1, &nativeBuffer, &byteOffset);
    }


    void CommandList::BindVertexBuffers(uint32_t startSlot, festd::span<RHI::Buffer* const> buffers,
                                        festd::span<const uint64_t> offsets)
    {
        FE_AssertMsg(buffers.size() == offsets.size(), "Number of buffers must be the same as number of offsets");

        festd::small_vector<VkBuffer> nativeBuffers(buffers.size(), VK_NULL_HANDLE);
        for (uint32_t i = 0; i < buffers.size(); ++i)
        {
            nativeBuffers[i] = NativeCast(buffers[i]);
        }

        vkCmdBindVertexBuffers(m_commandBuffer, startSlot, nativeBuffers.size(), nativeBuffers.data(), offsets.data());
    }


    void CommandList::CopyBuffers(RHI::Buffer* source, RHI::Buffer* dest, const RHI::BufferCopyRegion& region)
    {
        VkBufferCopy copy;
        copy.size = region.m_size;
        copy.dstOffset = region.m_destOffset;
        copy.srcOffset = region.m_sourceOffset;
        vkCmdCopyBuffer(m_commandBuffer, NativeCast(source), NativeCast(dest), 1, &copy);
    }


    void CommandList::CopyBufferToImage(RHI::Buffer* source, RHI::Image* dest, const RHI::BufferImageCopyRegion& region)
    {
        VkBufferImageCopy copy;
        copy.bufferOffset = region.m_bufferOffset;
        copy.bufferRowLength = 0;
        copy.bufferImageHeight = 0;

        const VkImageSubresource subresource = VKConvert(region.m_imageSubresource);
        copy.imageSubresource.aspectMask = subresource.aspectMask;
        copy.imageSubresource.mipLevel = subresource.mipLevel;
        copy.imageSubresource.baseArrayLayer = subresource.arrayLayer;
        copy.imageSubresource.layerCount = 1;

        copy.imageOffset = VKConvert(region.m_imageOffset);
        copy.imageExtent = VKConvert(region.m_imageSize);

        vkCmdCopyBufferToImage(
            m_commandBuffer, NativeCast(source), NativeCast(dest), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
    }


    void CommandList::BlitImage(RHI::Image* source, RHI::Image* dest, const RHI::ImageBlitRegion& region)
    {
        const VkImageSubresource srcSubresource = VKConvert(region.m_source);
        const VkImageSubresource dstSubresource = VKConvert(region.m_dest);

        VkImageBlit nativeBlit{};
        // SRC
        nativeBlit.srcSubresource.aspectMask = srcSubresource.aspectMask;
        nativeBlit.srcSubresource.baseArrayLayer = srcSubresource.arrayLayer;
        nativeBlit.srcSubresource.layerCount = 1;
        nativeBlit.srcSubresource.mipLevel = srcSubresource.mipLevel;

        nativeBlit.srcOffsets[0] = VKConvert(region.m_sourceBounds[0]);
        nativeBlit.srcOffsets[1] = VKConvert(region.m_sourceBounds[1]);

        // DEST
        nativeBlit.dstSubresource.aspectMask = dstSubresource.aspectMask;
        nativeBlit.dstSubresource.baseArrayLayer = dstSubresource.arrayLayer;
        nativeBlit.dstSubresource.layerCount = 1;
        nativeBlit.dstSubresource.mipLevel = dstSubresource.mipLevel;

        nativeBlit.dstOffsets[0] = VKConvert(region.m_destBounds[0]);
        nativeBlit.dstOffsets[1] = VKConvert(region.m_destBounds[1]);

        vkCmdBlitImage(m_commandBuffer,
                       NativeCast(source),
                       VKConvert(source->GetState(region.m_source)),
                       NativeCast(dest),
                       VKConvert(dest->GetState(region.m_dest)),
                       1,
                       &nativeBlit,
                       VK_FILTER_LINEAR);
    }


    CommandList::~CommandList()
    {
        vkFreeCommandBuffers(NativeCast(m_device), m_commandPool, 1, &m_commandBuffer);
    }
} // namespace FE::Graphics::Vulkan
