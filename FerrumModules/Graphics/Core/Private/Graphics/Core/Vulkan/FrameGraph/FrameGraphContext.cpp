#include <FeCore/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Vulkan/Base/Viewport.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/Fence.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Vulkan/GraphicsPipeline.h>
#include <Graphics/Core/Vulkan/Image.h>
#include <Graphics/Core/Vulkan/ShaderResourceGroup.h>
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


    FrameGraphContext::FrameGraphContext(Core::Device* device, Core::FrameGraph* frameGraph)
        : Common::FrameGraphContext(frameGraph)
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


    void FrameGraphContext::Submit()
    {
        FE_PROFILER_ZONE();

        m_resourceBarrierBatcher.Flush();
        VerifyVulkan(vkEndCommandBuffer(m_graphicsCommandBuffer->GetNative()));

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;

        const uint32_t waitSemaphoreCount = m_waitSemaphores.size() + m_waitFences.size();
        const uint32_t signalSemaphoreCount = m_signalSemaphores.size() + m_signalFences.size();

        Memory::FiberTempAllocator tempAllocator;

        festd::pmr::vector<VkSemaphore> waitSemaphores{ &tempAllocator };
        festd::pmr::vector<uint64_t> waitSemaphoreValues{ &tempAllocator };
        festd::pmr::vector<VkPipelineStageFlags> waitStageMasks{ &tempAllocator };
        waitSemaphores.reserve(waitSemaphoreCount);
        waitSemaphoreValues.reserve(waitSemaphoreCount);
        waitStageMasks.reserve(waitSemaphoreCount);

        for (const auto& [fence, value] : m_waitFences)
        {
            waitSemaphores.push_back(NativeCast(fence.Get()));
            waitSemaphoreValues.push_back(value);
            waitStageMasks.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        }

        for (const auto& [semaphore, stageMask] : m_waitSemaphores)
        {
            waitSemaphores.push_back(semaphore->GetNative());
            waitSemaphoreValues.push_back(0);
            waitStageMasks.push_back(stageMask);
        }

        festd::pmr::vector<VkSemaphore> signalSemaphores{ &tempAllocator };
        festd::pmr::vector<uint64_t> signalSemaphoreValues{ &tempAllocator };
        signalSemaphores.reserve(signalSemaphoreCount);
        signalSemaphoreValues.reserve(signalSemaphoreCount);

        for (const auto& [fence, value] : m_signalFences)
        {
            signalSemaphores.push_back(NativeCast(fence.Get()));
            signalSemaphoreValues.push_back(value);
        }

        for (const auto& semaphore : m_signalSemaphores)
        {
            signalSemaphores.push_back(semaphore->GetNative());
            signalSemaphoreValues.push_back(0);
        }

        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStageMasks.data();
        submitInfo.waitSemaphoreCount = waitSemaphoreCount;
        submitInfo.pSignalSemaphores = signalSemaphores.data();
        submitInfo.signalSemaphoreCount = signalSemaphoreCount;

        VkTimelineSemaphoreSubmitInfo timelineSemaphoreInfo = {};
        timelineSemaphoreInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;

        if (!m_waitFences.empty())
        {
            timelineSemaphoreInfo.waitSemaphoreValueCount = waitSemaphoreCount;
            timelineSemaphoreInfo.pWaitSemaphoreValues = waitSemaphoreValues.data();
            submitInfo.pNext = &timelineSemaphoreInfo;
        }

        if (!m_signalFences.empty())
        {
            timelineSemaphoreInfo.signalSemaphoreValueCount = signalSemaphoreCount;
            timelineSemaphoreInfo.pSignalSemaphoreValues = signalSemaphoreValues.data();
            submitInfo.pNext = &timelineSemaphoreInfo;
        }

        VkCommandBuffer commandBuffer = m_graphicsCommandBuffer->GetNative();
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.commandBufferCount = 1;

        const VkQueue queue = ImplCast(m_frameGraph->GetViewport())->GetQueue();
        VerifyVulkan(vkQueueSubmit(queue, 1, &submitInfo, nullptr));
    }


    void FrameGraphContext::DrawImpl(const Core::DrawList& drawList)
    {
        const VkCommandBuffer vkCommandBuffer = m_graphicsCommandBuffer->GetNative();

        m_resourceBarrierBatcher.Flush();

        if (m_viewportScissorState.m_dirty)
        {
            const VkViewport viewport = VKConvertViewport(m_viewportScissorState.m_viewport);
            vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);

            const VkRect2D scissor = VKConvertScissor(m_viewportScissorState.m_scissor);
            vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);
        }

        VkRenderingAttachmentInfo colorAttachments[Core::Limits::Pipeline::kMaxColorAttachments] = {};

        for (uint32_t rtIndex = 0; rtIndex < m_renderTargetState.m_renderTargetCount; ++rtIndex)
        {
            const Image* image = ImplCast(m_frameGraph->GetImage(m_renderTargetState.m_renderTargets[rtIndex]));
            FE_Assert(Bit::AllSet(image->GetDesc().m_bindFlags, Core::ImageBindFlags::kColorTarget));

            auto& attachmentInfo = colorAttachments[rtIndex];
            attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            attachmentInfo.imageView = image->GetWholeResourceView();
            attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            SetRenderingAttachmentInfo(
                attachmentInfo, rtIndex, m_renderTargetState.m_loadOperations, m_renderTargetState.m_storeOperations);
        }

        const Image* depthImage = ImplCast(m_frameGraph->GetImage(m_renderTargetState.m_depthStencil));
        FE_Assert(Bit::AllSet(depthImage->GetDesc().m_bindFlags, Core::ImageBindFlags::kDepthStencilTarget));

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = depthImage->GetWholeResourceView();
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        SetRenderingDepthAttachmentInfo(
            depthAttachment, m_renderTargetState.m_loadOperations, m_renderTargetState.m_storeOperations);

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = m_renderTargetState.m_renderTargetCount;
        renderingInfo.pColorAttachments = colorAttachments;
        renderingInfo.pDepthAttachment = &depthAttachment;
        renderingInfo.pStencilAttachment = &depthAttachment;
        renderingInfo.renderArea = VKConvertScissor(m_viewportScissorState.m_scissor);
        vkCmdBeginRendering(vkCommandBuffer, &renderingInfo);

        festd::small_vector<VkDescriptorSet> descriptorSets;
        for (uint32_t srgIndex = 0; srgIndex < drawList.m_sharedShaderResourceGroupCount; ++srgIndex)
        {
            const ShaderResourceGroup* srg = ImplCast(drawList.m_sharedShaderResourceGroups[srgIndex]);
            descriptorSets.push_back(srg->GetNativeSet());
        }

        for (uint32_t drawIndex = 0; drawIndex < drawList.m_drawCallCount; ++drawIndex)
        {
            const Core::DrawCall& drawCall = drawList.m_drawCalls[drawIndex];

            vkCmdSetStencilReference(vkCommandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, drawCall.m_stencilRef);

            for (uint32_t srgIndex = 0; srgIndex < drawCall.m_shaderResourceGroupCount; ++srgIndex)
            {
                const ShaderResourceGroup* srg = ImplCast(drawCall.m_shaderResourceGroups[srgIndex]);
                descriptorSets.push_back(srg->GetNativeSet());
            }

            const GraphicsPipeline* pipeline = ImplCast(drawCall.m_pipeline);

            if (!descriptorSets.empty())
            {
                vkCmdBindDescriptorSets(vkCommandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipeline->GetNativeLayout(),
                                        0,
                                        descriptorSets.size(),
                                        descriptorSets.data(),
                                        0,
                                        nullptr);
            }

            vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetNative());

            const Core::GraphicsPipelineDesc& pipelineDesc = pipeline->GetDesc();
            const Core::InputStreamLayout& inputLayout = pipelineDesc.m_inputLayout;

            const Core::GeometryView* geometryView = drawCall.m_geometryView;

            VkBuffer vertexBuffers[Core::Limits::Pipeline::kMaxVertexStreams] = {};
            VkDeviceSize vertexBufferOffsets[Core::Limits::Pipeline::kMaxVertexStreams] = {};

            uint32_t streamViewIndex = 0;
            const uint32_t activeStreamMask = inputLayout.CalculateActiveStreamMask();
            Bit::Traverse(activeStreamMask, [&](const uint32_t streamIndex) {
                const Core::StreamBufferView streamView = geometryView->m_streamBufferViews[streamViewIndex++];
                vertexBuffers[streamIndex] = NativeCast(streamView.m_buffer);
                vertexBufferOffsets[streamIndex] = streamView.m_byteOffset;
            });

            if (const uint32_t vertexBufferCount = Bit::PopCount(activeStreamMask); vertexBufferCount > 0)
                vkCmdBindVertexBuffers(vkCommandBuffer, 0, vertexBufferCount, vertexBuffers, vertexBufferOffsets);

            const Core::IndexBufferView indexView = geometryView->m_indexBufferView;
            if (indexView.m_byteSize > 0)
            {
                const VkBuffer indexBuffer = NativeCast(indexView.m_buffer);
                const VkIndexType indexType =
                    indexView.m_indexType == Core::IndexType::kUint16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;

                vkCmdBindIndexBuffer(vkCommandBuffer, indexBuffer, indexView.m_byteOffset, indexType);
            }

            const Core::DrawArguments drawArgs = geometryView->m_drawArguments;
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

            descriptorSets.resize(drawList.m_sharedShaderResourceGroupCount);
        }

        vkCmdEndRendering(vkCommandBuffer);
    }
} // namespace FE::Graphics::Vulkan
