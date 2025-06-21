#include <Graphics/Core/Common/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>

namespace FE::Graphics::Common
{
    FrameGraphContext::FrameGraphContext(Core::FrameGraph* frameGraph)
        : m_linearAllocator(frameGraph->GetAllocator())
        , m_signalFences(frameGraph->GetAllocator())
        , m_waitFences(frameGraph->GetAllocator())
    {
        m_frameGraph = frameGraph;
    }


    void FrameGraphContext::EnqueueFenceToWait(const Core::FenceSyncPoint& fence)
    {
        FE_Assert(fence.m_fence);
        m_waitFences.push_back(fence);
    }


    void FrameGraphContext::EnqueueFenceToSignal(const Core::FenceSyncPoint& fence)
    {
        FE_Assert(fence.m_fence);
        m_signalFences.push_back(fence);
    }


    void FrameGraphContext::SetRootConstants(const void* data, const uint32_t size)
    {
        FE_Assert(!Bit::AnySet(m_setStateMask, PipelineStateFlags::kRootConstants), "Root constants already set");
        m_setStateMask |= PipelineStateFlags::kRootConstants;

        memcpy(m_rootConstants, data, size);
        m_rootConstantsSize = size;
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


    void FrameGraphContext::SetViewportAndScissor(const Aabb& viewport, const RectInt scissor)
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


    void FrameGraphContext::Draw(const Core::DrawList& drawList)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Bit::AllSet(m_setStateMask, PipelineStateFlags::kAllRequired),
                  "All pipeline states must be set before drawing");
        DrawImpl(drawList);

        m_setStateMask = PipelineStateFlags::kNone;
        m_viewportScissorState.m_dirty = false;
    }
} // namespace FE::Graphics::Common
