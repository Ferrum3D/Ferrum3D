#include <FeCore/Containers/SmallVector.h>
#include <HAL/ImageView.h>
#include <HAL/Vulkan/Buffer.h>
#include <HAL/Vulkan/CommandList.h>
#include <HAL/Vulkan/Common/BaseTypes.h>
#include <HAL/Vulkan/Common/Viewport.h>
#include <HAL/Vulkan/DescriptorTable.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/Framebuffer.h>
#include <HAL/Vulkan/GraphicsPipeline.h>
#include <HAL/Vulkan/Image.h>
#include <HAL/Vulkan/ImageSubresource.h>
#include <HAL/Vulkan/RenderPass.h>
#include <HAL/Vulkan/ResourceState.h>

namespace FE::Graphics::Vulkan
{
    CommandList::CommandList(HAL::Device* pDevice)
        : m_IsUpdating(false)
    {
        m_pDevice = pDevice;
    }


    HAL::ResultCode CommandList::Init(const HAL::CommandListDesc& desc)
    {
        const VkDevice nativeDevice = NativeCast(m_pDevice);
        m_CommandPool = ImplCast(m_pDevice)->GetCommandPool(desc.QueueKind);
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = m_CommandPool;
        allocateInfo.commandBufferCount = 1;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        if (vkAllocateCommandBuffers(nativeDevice, &allocateInfo, &m_CommandBuffer) != VK_SUCCESS)
            return HAL::ResultCode::UnknownError;

        return HAL::ResultCode::Success;
    }


    void CommandList::Begin()
    {
        FE_ASSERT(!m_IsUpdating);
        m_IsUpdating = true;
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
    }


    void CommandList::End()
    {
        FE_ASSERT(m_IsUpdating);
        m_IsUpdating = false;
        vkEndCommandBuffer(m_CommandBuffer);
    }


    void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }


    void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                  uint32_t firstInstance)
    {
        vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }


    void CommandList::SetViewport(const HAL::Viewport& viewport)
    {
        const VkViewport nativeViewport = VKConvert(viewport);
        vkCmdSetViewport(m_CommandBuffer, 0, 1, &nativeViewport);
    }


    void CommandList::SetScissor(const HAL::Scissor& scissor)
    {
        const VkRect2D rect = VKConvert(scissor);
        vkCmdSetScissor(m_CommandBuffer, 0, 1, &rect);
    }


    void CommandList::ResourceTransitionBarriers(festd::span<const HAL::ImageBarrierDesc> imageBarriers,
                                                 festd::span<const HAL::BufferBarrierDesc> bufferBarriers)
    {
        festd::small_vector<VkImageMemoryBarrier> nativeImageBarriers;
        for (const HAL::ImageBarrierDesc& barrier : imageBarriers)
        {
            if (barrier.Image == nullptr)
                continue;

            auto stateBefore = barrier.StateBefore == HAL::ResourceState::Automatic
                ? barrier.Image->GetState(barrier.SubresourceRange.MinArraySlice, barrier.SubresourceRange.MinMipSlice)
                : barrier.StateBefore;

            const VkImageLayout before = VKConvert(stateBefore);
            const VkImageLayout after = VKConvert(barrier.StateAfter);
            if (before == after)
                continue;

            VkImageMemoryBarrier& imageMemoryBarrier = nativeImageBarriers.emplace_back();
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.oldLayout = before;
            imageMemoryBarrier.newLayout = after;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.image = NativeCast(barrier.Image);
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

        festd::small_vector<VkBufferMemoryBarrier> nativeBufferBarriers;
        for (const HAL::BufferBarrierDesc& barrier : bufferBarriers)
        {
            if (barrier.Buffer == nullptr)
                continue;

            FE_ASSERT_MSG(barrier.StateBefore != HAL::ResourceState::Automatic,
                          "Automatic resource state management is currently "
                          "not supported for buffers");

            if (barrier.StateBefore == barrier.StateAfter)
                continue;

            VkBufferMemoryBarrier& bufferMemoryBarrier = nativeBufferBarriers.emplace_back();
            bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.buffer = NativeCast(barrier.Buffer);
            bufferMemoryBarrier.offset = barrier.Offset;
            bufferMemoryBarrier.size = barrier.Size;

            bufferMemoryBarrier.srcAccessMask = GetAccessMask(barrier.StateBefore);
            bufferMemoryBarrier.dstAccessMask = GetAccessMask(barrier.StateAfter);
        }

        if (nativeImageBarriers.empty())
            return;

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


    void CommandList::MemoryBarrier()
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


    void CommandList::BindDescriptorTables(festd::span<HAL::DescriptorTable*> descriptorTables, HAL::GraphicsPipeline* pipeline)
    {
        festd::small_vector<VkDescriptorSet> nativeSets;
        for (const HAL::DescriptorTable* table : descriptorTables)
        {
            nativeSets.push_back(NativeCast(table));
        }

        vkCmdBindDescriptorSets(m_CommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                ImplCast(pipeline)->GetNativeLayout(),
                                0,
                                static_cast<uint32_t>(nativeSets.size()),
                                nativeSets.data(),
                                0,
                                nullptr);
    }


    void CommandList::BeginRenderPass(HAL::RenderPass* renderPass, HAL::Framebuffer* framebuffer,
                                      const festd::span<HAL::ClearValueDesc>& clearValues)
    {
        festd::small_vector<VkClearValue> vkClearValues{};
        for (const HAL::ClearValueDesc& clearValue : clearValues)
        {
            VkClearValue& vkClearValue = vkClearValues.emplace_back();
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
        info.framebuffer = NativeCast(framebuffer);
        info.renderPass = NativeCast(renderPass);
        info.clearValueCount = static_cast<uint32_t>(vkClearValues.size());
        info.pClearValues = vkClearValues.data();
        info.renderArea.offset = VkOffset2D{ 0, 0 };
        info.renderArea.extent = VkExtent2D{ framebuffer->GetDesc().Width, framebuffer->GetDesc().Height };
        vkCmdBeginRenderPass(m_CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }


    void CommandList::EndRenderPass()
    {
        vkCmdEndRenderPass(m_CommandBuffer);
    }


    void CommandList::BindGraphicsPipeline(HAL::GraphicsPipeline* pipeline)
    {
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, NativeCast(pipeline));
    }


    void CommandList::BindIndexBuffer(HAL::Buffer* buffer, uint64_t byteOffset)
    {
        vkCmdBindIndexBuffer(m_CommandBuffer, NativeCast(buffer), byteOffset, VK_INDEX_TYPE_UINT32);
    }


    void CommandList::BindVertexBuffer(uint32_t slot, HAL::Buffer* buffer, uint64_t byteOffset)
    {
        const VkBuffer nativeBuffer = NativeCast(buffer);
        vkCmdBindVertexBuffers(m_CommandBuffer, slot, 1, &nativeBuffer, &byteOffset);
    }


    void CommandList::BindVertexBuffers(uint32_t startSlot, festd::span<HAL::Buffer*> buffers,
                                        festd::span<const uint64_t> offsets)
    {
        FE_ASSERT_MSG(buffers.size() == offsets.size(), "Number of buffers must be the same as number of offsets");

        festd::small_vector<VkBuffer> nativeBuffers(buffers.size(), VK_NULL_HANDLE);
        for (uint32_t i = 0; i < buffers.size(); ++i)
        {
            nativeBuffers[i] = NativeCast(buffers[i]);
        }

        vkCmdBindVertexBuffers(
            m_CommandBuffer, startSlot, static_cast<uint32_t>(nativeBuffers.size()), nativeBuffers.data(), offsets.data());
    }


    void CommandList::CopyBuffers(HAL::Buffer* source, HAL::Buffer* dest, const HAL::BufferCopyRegion& region)
    {
        VkBufferCopy copy{};
        copy.size = region.Size;
        copy.dstOffset = region.DestOffset;
        copy.srcOffset = region.SourceOffset;
        vkCmdCopyBuffer(m_CommandBuffer, NativeCast(source), NativeCast(dest), 1, &copy);
    }


    void CommandList::CopyBufferToImage(HAL::Buffer* source, HAL::Image* dest, const HAL::BufferImageCopyRegion& region)
    {
        VkBufferImageCopy copy{};
        copy.bufferOffset = region.BufferOffset;
        copy.bufferRowLength = 0;
        copy.bufferImageHeight = 0;

        const VkImageSubresource subresource = VKConvert(region.ImageSubresource);
        copy.imageSubresource.aspectMask = subresource.aspectMask;
        copy.imageSubresource.mipLevel = subresource.mipLevel;
        copy.imageSubresource.baseArrayLayer = subresource.arrayLayer;
        copy.imageSubresource.layerCount = 1;

        copy.imageOffset = VKConvert(region.ImageOffset);
        copy.imageExtent = VKConvert(region.ImageSize);

        vkCmdCopyBufferToImage(
            m_CommandBuffer, NativeCast(source), NativeCast(dest), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
    }


    void CommandList::BlitImage(HAL::Image* source, HAL::Image* dest, const HAL::ImageBlitRegion& region)
    {
        const VkImageSubresource srcSubresource = VKConvert(region.Source);
        const VkImageSubresource dstSubresource = VKConvert(region.Dest);

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
                       NativeCast(source),
                       VKConvert(source->GetState(region.Source)),
                       NativeCast(dest),
                       VKConvert(dest->GetState(region.Dest)),
                       1,
                       &nativeBlit,
                       VK_FILTER_LINEAR);
    }


    CommandList::~CommandList()
    {
        vkFreeCommandBuffers(NativeCast(m_pDevice), m_CommandPool, 1, &m_CommandBuffer);
    }
} // namespace FE::Graphics::Vulkan
