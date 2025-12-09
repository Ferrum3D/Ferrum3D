#include <Graphics/Core/Vulkan/Base/Viewport.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/ComputePipeline.h>
#include <Graphics/Core/Vulkan/DescriptorManager.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Vulkan/GraphicsPipeline.h>
#include <Graphics/Core/Vulkan/Texture.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        void SetRenderingAttachmentInfo(VkRenderingAttachmentInfo& attachmentInfo, const uint32_t renderTargetIndex,
                                        const Core::RenderTargetLoadOperations& loadOperations,
                                        const Core::RenderTargetStoreOperations& storeOperations)
        {
            attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            if (loadOperations.m_colorClearMask & (1 << renderTargetIndex))
            {
                attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                memcpy(attachmentInfo.clearValue.color.float32,
                       loadOperations.m_colorClearValues[renderTargetIndex].Data(),
                       sizeof(Color4F));
            }

            if (loadOperations.m_colorDiscardMask & (1 << renderTargetIndex))
                attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

            attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            if (storeOperations.m_colorDiscardMask & (1 << renderTargetIndex))
                attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }


        void SetRenderingDepthAttachmentInfo(VkRenderingAttachmentInfo& attachmentInfo,
                                             const Core::RenderTargetLoadOperations& loadOperations,
                                             const Core::RenderTargetStoreOperations& storeOperations)
        {
            attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            if (loadOperations.m_depthStencilCleared)
            {
                attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachmentInfo.clearValue.depthStencil.depth = loadOperations.m_depthClearValue;
                attachmentInfo.clearValue.depthStencil.stencil = loadOperations.m_stencilClearValue;
            }

            if (loadOperations.m_depthStencilDiscarded)
                attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

            attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            if (storeOperations.m_depthStencilDiscarded)
                attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
    } // namespace


    FrameGraphContext::FrameGraphContext(Core::Device* device, Core::FrameGraph* frameGraph,
                                         Core::DescriptorManager* descriptorManager)
        : Common::FrameGraphContext(frameGraph)
        , m_descriptorManager(ImplCast(descriptorManager))
    {
        m_device = device;
    }


    void FrameGraphContext::Init(CommandBuffer* graphicsCommandBuffer)
    {
        m_graphicsCommandBuffer = graphicsCommandBuffer;
    }


    void FrameGraphContext::BeginRendering(const VkCommandBuffer vkCommandBuffer) const
    {
        if (m_viewportScissorState.m_dirty)
        {
            const VkViewport viewport = TranslateViewport(m_viewportScissorState.m_viewport);
            vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);

            const VkRect2D scissor = TranslateScissor(m_viewportScissorState.m_scissor);
            vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);
        }

        VkRenderingAttachmentInfo colorAttachments[Core::Limits::Pipeline::kMaxColorAttachments] = {};

        for (uint32_t rtIndex = 0; rtIndex < m_renderTargetState.m_renderTargetCount; ++rtIndex)
        {
            const Core::TextureView& renderTargetView = m_renderTargetState.m_renderTargets[rtIndex];
            const Texture* image = ImplCast(renderTargetView.m_resource);
            const Core::FormatInfo formatInfo{ image->GetDesc().m_imageFormat };
            FE_Assert(formatInfo.m_aspectFlags == Core::ImageAspect::kColor);
            FE_Assert(renderTargetView.m_subresource.m_mipSliceCount == 1);
            FE_Assert(renderTargetView.m_subresource.m_arraySize == 1);

            auto& attachmentInfo = colorAttachments[rtIndex];
            attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            attachmentInfo.imageView = image->GetSubresourceView(renderTargetView.m_subresource);
            attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            SetRenderingAttachmentInfo(attachmentInfo,
                                       rtIndex,
                                       m_renderTargetState.m_loadOperations,
                                       m_renderTargetState.m_storeOperations);
        }

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;

        VkRenderingAttachmentInfo depthAttachment{};
        if (m_renderTargetState.m_depthStencil.IsValid())
        {
            const Core::TextureView& depthStencilView = m_renderTargetState.m_depthStencil;
            const Texture* image = ImplCast(depthStencilView.m_resource);
            const Core::FormatInfo formatInfo{ image->GetDesc().m_imageFormat };
            FE_Assert(Bit::AnySet(formatInfo.m_aspectFlags, Core::ImageAspect::kDepthStencil));
            FE_Assert(depthStencilView.m_subresource.m_mipSliceCount == 1);
            FE_Assert(depthStencilView.m_subresource.m_arraySize == 1);

            depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depthAttachment.imageView = image->GetSubresourceView(depthStencilView.m_subresource);
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            SetRenderingDepthAttachmentInfo(depthAttachment,
                                            m_renderTargetState.m_loadOperations,
                                            m_renderTargetState.m_storeOperations);

            renderingInfo.pDepthAttachment = &depthAttachment;
            renderingInfo.pStencilAttachment = &depthAttachment;
        }

        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = m_renderTargetState.m_renderTargetCount;
        renderingInfo.pColorAttachments = colorAttachments;
        renderingInfo.renderArea = TranslateScissor(m_viewportScissorState.m_scissor);
        vkCmdBeginRendering(vkCommandBuffer, &renderingInfo);
    }


    void FrameGraphContext::DrawImpl(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t indexOffset,
                                     const uint32_t vertexOffset, const uint32_t instanceOffset)
    {
        const VkCommandBuffer vkCommandBuffer = m_graphicsCommandBuffer->GetNative();
        BeginRendering(vkCommandBuffer);

        if (m_stencilRefState.m_dirty)
            vkCmdSetStencilReference(vkCommandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, m_stencilRefState.m_stencilRef);

        const GraphicsPipeline* pipeline = ImplCast(m_pipelineState.m_graphicsPipeline);
        FE_Assert(pipeline->IsReady());

        const VkDescriptorSet descriptorSet = m_descriptorManager->GetDescriptorSet();
        const VkPipelineLayout pipelineLayout = pipeline->GetNativeLayout();
        if (descriptorSet && m_pipelineState.m_dirty)
        {
            vkCmdBindDescriptorSets(vkCommandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayout,
                                    0,
                                    1,
                                    &descriptorSet,
                                    0,
                                    nullptr);
        }

        if (Bit::AllSet(m_setStateMask, Common::PipelineStateFlags::kPushConstants))
            vkCmdPushConstants(vkCommandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, m_pushConstantsSize, m_pushConstants);

        if (m_pipelineState.m_dirty)
            vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetNative());

        const Core::GraphicsPipelineDesc& pipelineDesc = pipeline->GetDesc();
        const Core::InputStreamLayout& inputLayout = pipelineDesc.m_inputLayout;

        VkBuffer vertexBuffers[Core::Limits::Pipeline::kMaxVertexStreams] = {};
        VkDeviceSize vertexBufferOffsets[Core::Limits::Pipeline::kMaxVertexStreams] = {};

        uint32_t streamViewIndex = 0;
        const uint32_t activeStreamMask = inputLayout.CalculateActiveStreamMask();
        Bit::Traverse(activeStreamMask, [&](const uint32_t streamIndex) {
            const Core::BufferView streamView = m_streamBufferViews[streamViewIndex++];
            FE_Assert(streamView.IsValid());
            vertexBuffers[streamIndex] = NativeCast(streamView.m_resource);
            vertexBufferOffsets[streamIndex] = streamView.m_subresource.m_offset;
        });

        if (const uint32_t vertexBufferCount = Bit::PopCount(activeStreamMask); vertexBufferCount > 0)
            vkCmdBindVertexBuffers(vkCommandBuffer, 0, vertexBufferCount, vertexBuffers, vertexBufferOffsets);

        if (m_indexBufferView.IsValid())
        {
            const VkBuffer indexBuffer = NativeCast(m_indexBufferView.m_resource);
            const VkIndexType indexType = m_indexType == Core::IndexType::kUint16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
            vkCmdBindIndexBuffer(vkCommandBuffer, indexBuffer, m_indexBufferView.m_subresource.m_offset, indexType);
        }

        vkCmdDrawIndexed(vkCommandBuffer,
                         indexCount,
                         instanceCount,
                         indexOffset,
                         static_cast<int32_t>(vertexOffset),
                         instanceOffset);

        vkCmdEndRendering(vkCommandBuffer);
    }


    void FrameGraphContext::DispatchMeshImpl(const Vector3UInt workGroupCount)
    {
        const GraphicsPipeline* pipelineImpl = ImplCast(m_pipelineState.m_graphicsPipeline);
        FE_Assert(pipelineImpl->IsReady());

        const VkCommandBuffer vkCommandBuffer = m_graphicsCommandBuffer->GetNative();
        BeginRendering(vkCommandBuffer);

        if (m_stencilRefState.m_dirty)
            vkCmdSetStencilReference(vkCommandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, m_stencilRefState.m_stencilRef);

        const VkDescriptorSet descriptorSet = m_descriptorManager->GetDescriptorSet();
        const VkPipelineLayout pipelineLayout = pipelineImpl->GetNativeLayout();
        if (descriptorSet && m_pipelineState.m_dirty)
        {
            vkCmdBindDescriptorSets(vkCommandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayout,
                                    0,
                                    1,
                                    &descriptorSet,
                                    0,
                                    nullptr);
        }

        if (Bit::AllSet(m_setStateMask, Common::PipelineStateFlags::kPushConstants))
            vkCmdPushConstants(vkCommandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, m_pushConstantsSize, m_pushConstants);

        if (m_pipelineState.m_dirty)
            vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineImpl->GetNative());

        vkCmdDrawMeshTasksEXT(vkCommandBuffer, workGroupCount.x, workGroupCount.y, workGroupCount.z);

        vkCmdEndRendering(vkCommandBuffer);
    }


    void FrameGraphContext::DispatchImpl(const Vector3UInt workGroupCount)
    {
        const ComputePipeline* pipelineImpl = ImplCast(m_pipelineState.m_computePipeline);
        FE_Assert(pipelineImpl->IsReady());

        const VkCommandBuffer vkCommandBuffer = m_graphicsCommandBuffer->GetNative();

        const VkPipelineLayout pipelineLayout = pipelineImpl->GetNativeLayout();
        const VkDescriptorSet descriptorSet = m_descriptorManager->GetDescriptorSet();
        if (descriptorSet && m_pipelineState.m_dirty)
        {
            vkCmdBindDescriptorSets(vkCommandBuffer,
                                    VK_PIPELINE_BIND_POINT_COMPUTE,
                                    pipelineLayout,
                                    0,
                                    1,
                                    &descriptorSet,
                                    0,
                                    nullptr);
        }

        if (Bit::AllSet(m_setStateMask, Common::PipelineStateFlags::kPushConstants))
            vkCmdPushConstants(vkCommandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, m_pushConstantsSize, m_pushConstants);

        if (m_pipelineState.m_dirty)
            vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineImpl->GetNative());

        vkCmdDispatch(vkCommandBuffer, workGroupCount.x, workGroupCount.y, workGroupCount.z);
    }


    void FrameGraphContext::EnqueueFenceToSignal(const Core::FenceSyncPoint& fence)
    {
        m_graphicsCommandBuffer->EnqueueFenceToSignal(fence);
    }


    void FrameGraphContext::EnqueueFenceToWait(const Core::FenceSyncPoint& fence)
    {
        m_graphicsCommandBuffer->EnqueueFenceToWait(fence);
    }
} // namespace FE::Graphics::Vulkan
