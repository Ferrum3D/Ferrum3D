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


    void FrameGraphContext::SetRenderTargets(const festd::span<const Core::RenderTargetHandle> renderTargets,
                                             const Core::RenderTargetHandle depthStencil)
    {
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kRenderTargets), "Render targets already set");
        m_setStateMask |= PipelineStateFlags::kRenderTargets;

        for (Core::RenderTargetHandle handle : renderTargets)
        {
            if (handle.IsValid())
                FE_Assert(handle.m_desc.m_accessType == festd::to_underlying(Core::ImageWriteType::kColorTarget));
        }

        if (depthStencil.IsValid())
            FE_Assert(depthStencil.m_desc.m_accessType == festd::to_underlying(Core::ImageWriteType::kDepthStencilTarget));

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
            m_viewportScissorState.m_dirty = true;
            m_viewportScissorState.m_viewport = viewport;
            m_viewportScissorState.m_scissor = scissor;
        }
    }


    void FrameGraphContext::Draw(const Core::DrawCall& drawCall)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Bit::AllSet(m_setStateMask, PipelineStateFlags::kAllRequiredForGraphics),
                  "All pipeline states must be set before drawing");
        DrawImpl(drawCall);

        m_setStateMask = PipelineStateFlags::kNone;
        m_viewportScissorState.m_dirty = false;
    }


    void FrameGraphContext::DispatchMesh(const Core::GraphicsPipeline* pipeline, Vector3UInt workGroupCount, uint32_t stencilRef)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Bit::AllSet(m_setStateMask, PipelineStateFlags::kAllRequiredForGraphics),
                  "All pipeline states must be set before dispatching mesh shader");
        DispatchMeshImpl(pipeline, workGroupCount, stencilRef);

        m_setStateMask = PipelineStateFlags::kNone;
        m_viewportScissorState.m_dirty = false;
    }


    void FrameGraphContext::Dispatch(const Core::ComputePipeline* pipeline, const Vector3UInt workGroupCount)
    {
        FE_PROFILER_ZONE();

        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kAllRequiredForGraphics),
                  "Only compute related pipeline states can be set before dispatching");
        DispatchImpl(pipeline, workGroupCount);

        m_setStateMask = PipelineStateFlags::kNone;
        m_viewportScissorState.m_dirty = false;
    }
} // namespace FE::Graphics::Common
