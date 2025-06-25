#include <Graphics/Core/Vulkan/Base/Viewport.h>
#include <Graphics/Core/Vulkan/BindlessManager.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/ComputePipeline.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Vulkan/GraphicsPipeline.h>
#include <Graphics/Core/Vulkan/RenderTarget.h>
#include <Graphics/Core/Vulkan/Viewport.h>

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


    FrameGraphContext::FrameGraphContext(Core::Device* device, Core::FrameGraph* frameGraph, BindlessManager* bindlessManager)
        : Common::FrameGraphContext(frameGraph)
        , m_bindlessManager(bindlessManager)
        , m_resourceBarrierBatcher(device)
        , m_signalSemaphores(frameGraph->GetAllocator())
        , m_waitSemaphores(frameGraph->GetAllocator())
    {
        m_device = device;
    }


    void FrameGraphContext::Init(CommandBuffer* graphicsCommandBuffer)
    {
        m_graphicsCommandBuffer = graphicsCommandBuffer;
        m_resourceBarrierBatcher.Begin(m_graphicsCommandBuffer->GetNative());
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
            const RenderTarget* image = ImplCast(m_frameGraph->GetRenderTarget(m_renderTargetState.m_renderTargets[rtIndex]));
            const Core::FormatInfo formatInfo{ image->GetDesc().m_imageFormat };
            FE_Assert(formatInfo.m_aspectFlags == Core::ImageAspect::kColor);

            auto& attachmentInfo = colorAttachments[rtIndex];
            attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            attachmentInfo.imageView = image->GetWholeResourceView();
            attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            SetRenderingAttachmentInfo(
                attachmentInfo, rtIndex, m_renderTargetState.m_loadOperations, m_renderTargetState.m_storeOperations);
        }

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;

        if (m_renderTargetState.m_depthStencil.IsValid())
        {
            const RenderTarget* depthImage = ImplCast(m_frameGraph->GetRenderTarget(m_renderTargetState.m_depthStencil));
            const Core::FormatInfo formatInfo{ depthImage->GetDesc().m_imageFormat };
            FE_Assert(Bit::AnySet(formatInfo.m_aspectFlags, Core::ImageAspect::kDepthStencil));

            VkRenderingAttachmentInfo depthAttachment{};
            depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depthAttachment.imageView = depthImage->GetWholeResourceView();
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            SetRenderingDepthAttachmentInfo(
                depthAttachment, m_renderTargetState.m_loadOperations, m_renderTargetState.m_storeOperations);

            renderingInfo.pDepthAttachment = &depthAttachment;
            renderingInfo.pStencilAttachment = &depthAttachment;
        }

        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = m_renderTargetState.m_renderTargetCount;
        renderingInfo.pColorAttachments = colorAttachments;
        renderingInfo.renderArea = TranslateScissor(m_viewportScissorState.m_scissor);
        vkCmdBeginRendering(vkCommandBuffer, &renderingInfo);
    }


    void FrameGraphContext::DrawImpl(const Core::DrawCall& drawCall)
    {
        const VkCommandBuffer vkCommandBuffer = m_graphicsCommandBuffer->GetNative();
        BeginRendering(vkCommandBuffer);

        vkCmdSetStencilReference(vkCommandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, drawCall.m_stencilRef);

        const GraphicsPipeline* pipeline = ImplCast(drawCall.m_pipeline);
        FE_Assert(pipeline->IsReady());

        const VkDescriptorSet descriptorSet = m_bindlessManager->GetDescriptorSet();
        const VkPipelineLayout pipelineLayout = pipeline->GetNativeLayout();
        if (descriptorSet)
        {
            vkCmdBindDescriptorSets(
                vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        }

        if (Bit::AllSet(m_setStateMask, Common::PipelineStateFlags::kPushConstants))
        {
            vkCmdPushConstants(vkCommandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, m_pushConstantsSize, m_pushConstants);
        }

        vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetNative());

        const Core::GraphicsPipelineDesc& pipelineDesc = pipeline->GetDesc();
        const Core::InputStreamLayout& inputLayout = pipelineDesc.m_inputLayout;

        const Core::GeometryView& geometryView = drawCall.m_geometryView;

        VkBuffer vertexBuffers[Core::Limits::Pipeline::kMaxVertexStreams] = {};
        VkDeviceSize vertexBufferOffsets[Core::Limits::Pipeline::kMaxVertexStreams] = {};

        uint32_t streamViewIndex = 0;
        const uint32_t activeStreamMask = inputLayout.CalculateActiveStreamMask();
        Bit::Traverse(activeStreamMask, [&](const uint32_t streamIndex) {
            const Core::StreamBufferView streamView = geometryView.m_streamBufferViews[streamViewIndex++];
            vertexBuffers[streamIndex] = NativeCast(streamView.m_buffer);
            vertexBufferOffsets[streamIndex] = streamView.m_byteOffset;
        });

        if (const uint32_t vertexBufferCount = Bit::PopCount(activeStreamMask); vertexBufferCount > 0)
            vkCmdBindVertexBuffers(vkCommandBuffer, 0, vertexBufferCount, vertexBuffers, vertexBufferOffsets);

        const Core::IndexBufferView indexView = geometryView.m_indexBufferView;
        if (indexView.m_byteSize > 0)
        {
            const VkBuffer indexBuffer = NativeCast(indexView.m_buffer);
            const VkIndexType indexType =
                indexView.m_indexType == Core::IndexType::kUint16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;

            vkCmdBindIndexBuffer(vkCommandBuffer, indexBuffer, indexView.m_byteOffset, indexType);
        }

        const Core::DrawArguments drawArgs = geometryView.m_drawArguments;
        switch (drawArgs.m_type)
        {
        case Core::DrawArgumentsType::kLinear:
            {
                const Core::DrawArgumentsLinear& args = drawArgs.m_linear;
                vkCmdDraw(vkCommandBuffer,
                          args.m_vertexCount,
                          drawCall.m_instanceCount,
                          args.m_vertexOffset,
                          drawCall.m_instanceOffset);
            }
            break;
        case Core::DrawArgumentsType::kIndexed:
            {
                const Core::DrawArgumentsIndexed& args = drawArgs.m_indexed;
                vkCmdDrawIndexed(vkCommandBuffer,
                                 drawArgs.m_indexed.m_indexCount,
                                 drawCall.m_instanceCount,
                                 args.m_indexOffset,
                                 static_cast<int32_t>(args.m_vertexOffset),
                                 drawCall.m_instanceOffset);
            }
            break;
        default:
            FE_DebugBreak();
            break;
        }

        vkCmdEndRendering(vkCommandBuffer);
    }


    void FrameGraphContext::DispatchMeshImpl(const Core::GraphicsPipeline* pipeline, Vector3UInt workGroupCount,
                                             uint32_t stencilRef)
    {
        const GraphicsPipeline* pipelineImpl = ImplCast(pipeline);
        FE_Assert(pipelineImpl->IsReady());

        const VkCommandBuffer vkCommandBuffer = m_graphicsCommandBuffer->GetNative();
        BeginRendering(vkCommandBuffer);

        vkCmdSetStencilReference(vkCommandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, stencilRef);

        const VkDescriptorSet descriptorSet = m_bindlessManager->GetDescriptorSet();
        const VkPipelineLayout pipelineLayout = pipelineImpl->GetNativeLayout();
        if (descriptorSet)
        {
            vkCmdBindDescriptorSets(
                vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        }

        if (Bit::AllSet(m_setStateMask, Common::PipelineStateFlags::kPushConstants))
        {
            vkCmdPushConstants(vkCommandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, m_pushConstantsSize, m_pushConstants);
        }

        vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineImpl->GetNative());

        vkCmdDrawMeshTasksEXT(vkCommandBuffer, workGroupCount.x, workGroupCount.y, workGroupCount.z);

        vkCmdEndRendering(vkCommandBuffer);
    }


    void FrameGraphContext::DispatchImpl(const Core::ComputePipeline* pipeline, const Vector3UInt workGroupCount)
    {
        const ComputePipeline* pipelineImpl = ImplCast(pipeline);
        FE_Assert(pipelineImpl->IsReady());

        const VkCommandBuffer vkCommandBuffer = m_graphicsCommandBuffer->GetNative();

        const VkPipelineLayout pipelineLayout = pipelineImpl->GetNativeLayout();
        const VkDescriptorSet descriptorSet = m_bindlessManager->GetDescriptorSet();
        if (descriptorSet)
        {
            vkCmdBindDescriptorSets(
                vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        }

        if (Bit::AllSet(m_setStateMask, Common::PipelineStateFlags::kPushConstants))
        {
            vkCmdPushConstants(vkCommandBuffer, pipelineLayout, VK_SHADER_STAGE_ALL, 0, m_pushConstantsSize, m_pushConstants);
        }

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
