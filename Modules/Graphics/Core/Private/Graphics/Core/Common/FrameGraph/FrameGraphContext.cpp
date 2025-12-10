#include <Graphics/Core/Common/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>

namespace FE::Graphics::Common
{
    FrameGraphContext::FrameGraphContext(Core::FrameGraph* frameGraph)
        : m_linearAllocator(frameGraph->GetAllocator())
    {
        m_frameGraph = frameGraph;
    }


    void FrameGraphContext::PushConstants(const void* data, const uint32_t size)
    {
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kPushConstants), "Push constants already set");
        m_setStateMask |= PipelineStateFlags::kPushConstants;

        memcpy(m_pushConstants, data, size);
        m_pushConstantsSize = size;
    }


    void FrameGraphContext::SetRenderTargetLoadOperations(const Core::RenderTargetLoadOperations& operations)
    {
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kLoadOperations), "Load operations already set");
        m_setStateMask |= PipelineStateFlags::kLoadOperations;

        m_renderTargetState.m_loadOperations = operations;
    }


    void FrameGraphContext::SetRenderTargetStoreOperations(const Core::RenderTargetStoreOperations& operations)
    {
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kStoreOperations), "Store operations already set");
        m_setStateMask |= PipelineStateFlags::kStoreOperations;

        m_renderTargetState.m_storeOperations = operations;
    }


    void FrameGraphContext::SetRenderTargets(const festd::span<const Core::TextureView> renderTargets,
                                             const Core::TextureView depthStencil)
    {
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kRenderTargets), "Render targets already set");
        m_setStateMask |= PipelineStateFlags::kRenderTargets;

        festd::copy(renderTargets.begin(), renderTargets.end(), m_renderTargetState.m_renderTargets);
        m_renderTargetState.m_renderTargetCount = renderTargets.size();
        m_renderTargetState.m_depthStencil = depthStencil;
    }


    void FrameGraphContext::SetViewportAndScissor(const RectF viewport, const RectInt scissor)
    {
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kViewportScissor), "Viewport and scissor already set");
        m_setStateMask |= PipelineStateFlags::kViewportScissor;

        if (!Math::EqualEstimate(m_viewportScissorState.m_viewport, viewport) || m_viewportScissorState.m_scissor != scissor)
        {
            m_viewportScissorState.m_action = StateAction::kSet;
            m_viewportScissorState.m_viewport = viewport;
            m_viewportScissorState.m_scissor = scissor;
        }
        else
        {
            m_viewportScissorState.m_action = StateAction::kKeep;
        }
    }


    void FrameGraphContext::SetPipeline(const Core::GraphicsPipeline* pipeline)
    {
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kGraphicsPipeline | PipelineStateFlags::kComputePipeline),
                  "Pipeline already set");
        m_setStateMask |= PipelineStateFlags::kGraphicsPipeline;

        if (m_pipelineState.m_graphicsPipeline != pipeline)
        {
            m_pipelineState.m_graphicsPipeline = pipeline;
            m_pipelineState.m_computePipeline = nullptr;
            m_pipelineState.m_action = StateAction::kSet;
        }
        else
        {
            m_pipelineState.m_action = StateAction::kKeep;
        }

        FE_AssertDebug(m_pipelineState.m_computePipeline == nullptr);
    }


    void FrameGraphContext::SetPipeline(const Core::ComputePipeline* pipeline)
    {
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kGraphicsPipeline | PipelineStateFlags::kComputePipeline),
                  "Pipeline already set");
        m_setStateMask |= PipelineStateFlags::kComputePipeline;

        if (m_pipelineState.m_computePipeline != pipeline)
        {
            m_pipelineState.m_computePipeline = pipeline;
            m_pipelineState.m_graphicsPipeline = nullptr;
            m_pipelineState.m_action = StateAction::kSet;
        }
        else
        {
            m_pipelineState.m_action = StateAction::kKeep;
        }

        FE_AssertDebug(m_pipelineState.m_graphicsPipeline == nullptr);
    }


    void FrameGraphContext::SetStencilRef(const uint8_t stencilRef)
    {
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kStencilRef), "Stencil ref already set");
        m_setStateMask |= PipelineStateFlags::kStencilRef;

        if (m_stencilRefState.m_stencilRef != stencilRef)
        {
            m_stencilRefState.m_stencilRef = stencilRef;
            m_stencilRefState.m_action = StateAction::kSet;
        }
        else
        {
            m_pipelineState.m_action = StateAction::kKeep;
        }
    }


    void FrameGraphContext::SetStreamBuffers(const festd::span<const Core::BufferView> bufferViews)
    {
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kStreamBuffers), "Stream buffers already set");
        m_setStateMask |= PipelineStateFlags::kStreamBuffers;

        festd::copy(bufferViews.begin(), bufferViews.end(), m_streamBufferViews);
    }


    void FrameGraphContext::SetIndexBuffer(const Core::BufferView bufferView, const Core::IndexType indexType)
    {
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kIndexBuffer), "Index buffer already set");
        m_setStateMask |= PipelineStateFlags::kIndexBuffer;

        m_indexBufferView = bufferView;
        m_indexType = indexType;
    }


    void FrameGraphContext::Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t vertexOffset,
                                 const uint32_t instanceOffset)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Bit::AllSet(m_setStateMask, PipelineStateFlags::kAllRequiredForGraphics),
                  "All pipeline states must be set before drawing");
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kComputePipeline),
                  "Compute pipeline must not be set when drawing");

        PrepareStatesInternal();
        DrawImpl(vertexCount, instanceCount, vertexOffset, instanceOffset);
        ClearStatesInternal();
    }


    void FrameGraphContext::DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t indexOffset,
                                        const uint32_t vertexOffset, const uint32_t instanceOffset)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Bit::AllSet(m_setStateMask, PipelineStateFlags::kAllRequiredForGraphics),
                  "All pipeline states must be set before drawing");
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kComputePipeline),
                  "Compute pipeline must not be set when drawing");

        PrepareStatesInternal();
        DrawIndexedImpl(indexCount, instanceCount, indexOffset, vertexOffset, instanceOffset);
        ClearStatesInternal();
    }


    void FrameGraphContext::DispatchMesh(const Core::ComputeWorkGroupCount workGroupCount)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Bit::AllSet(m_setStateMask, PipelineStateFlags::kAllRequiredForGraphics),
                  "All pipeline states must be set before drawing");
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kComputePipeline),
                  "Compute pipeline must not be set when drawing");

        PrepareStatesInternal();
        DispatchMeshImpl(workGroupCount.m_workGroupCount);
        ClearStatesInternal();
    }


    void FrameGraphContext::Dispatch(const Core::ComputeWorkGroupCount workGroupCount)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Bit::AllSet(m_setStateMask, PipelineStateFlags::kComputePipeline),
                  "Compute pipeline must be set before dispatching");
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kAllRequiredForGraphics),
                  "Only compute related pipeline states can be set before dispatching");

        PrepareStatesInternal();
        DispatchImpl(workGroupCount.m_workGroupCount);
        ClearStatesInternal();
    }


    bool FrameGraphContext::IsCleanState() const
    {
        return m_setStateMask == PipelineStateFlags::kNone;
    }


    void FrameGraphContext::ClearStatesInternal()
    {
        m_setStateMask = PipelineStateFlags::kNone;
        m_viewportScissorState.m_action = StateAction::kReset;
        m_pipelineState.m_action = StateAction::kReset;
        m_stencilRefState.m_action = StateAction::kReset;

        festd::fill_n(m_streamBufferViews, festd::size(m_streamBufferViews), Core::BufferView::kInvalid);
        m_indexBufferView = {};
    }


    void FrameGraphContext::PrepareStatesInternal()
    {
        FE_AssertDebug(m_pipelineState.m_action != StateAction::kReset, "Must have been checked by the caller");

        if (m_viewportScissorState.m_action == StateAction::kReset)
        {
            if (Bit::AllSet(m_setStateMask, PipelineStateFlags::kRenderTargets))
            {
                const Vector2UInt size = m_renderTargetState.m_renderTargets[0].GetBaseDesc().GetSize2D();
                const auto viewport = RectF::FromPosAndSize({ 0.0f, 0.0f }, Vector2(size));
                const auto scissor = RectInt::FromPosAndSize({ 0, 0 }, Vector2Int(size));
                SetViewportAndScissor(viewport, scissor);
            }
            else if (Bit::AllSet(m_setStateMask, PipelineStateFlags::kComputePipeline))
            {
                // Compute pipeline should not care about viewport and scissor.
                m_viewportScissorState.m_action = StateAction::kKeep;
            }
            else
            {
                FE_DebugBreak();
            }
        }

        if (m_stencilRefState.m_action == StateAction::kReset)
        {
            if (Bit::AllSet(m_setStateMask, PipelineStateFlags::kGraphicsPipeline))
            {
                SetStencilRef(0);
            }
            else
            {
                m_stencilRefState.m_action = StateAction::kKeep;
            }
        }
    }
} // namespace FE::Graphics::Common
