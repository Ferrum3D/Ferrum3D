#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/FrameGraph/FrameGraphContext.h>

namespace FE::Graphics::Common
{
    enum class PipelineStateFlags : uint32_t
    {
        kNone = 0,
        kLoadOperations = 1 << 0,
        kStoreOperations = 1 << 1,
        kRenderTargets = 1 << 2,
        kViewportScissor = 1 << 3,
        kRootConstants = 1 << 4,
        kAllRequired = kLoadOperations | kStoreOperations | kRenderTargets | kViewportScissor,
        kAll = kAllRequired | kRootConstants,
    };

    FE_ENUM_OPERATORS(PipelineStateFlags);


    struct FrameGraphContext : public Core::FrameGraphContext
    {
        FE_RTTI_Class(FrameGraphContext, "521A8CCE-6A61-4D51-962C-16ABAB20AE89");

        void EnqueueFenceToWait(const Core::FenceSyncPoint& fence) final;

        void EnqueueFenceToSignal(const Core::FenceSyncPoint& fence) final;

        void SetRootConstants(const void* data, uint32_t size) override;

        void SetRenderTargetLoadOperations(const Core::RenderTargetLoadOperations& operations) final;

        void SetRenderTargetStoreOperations(const Core::RenderTargetStoreOperations& operations) final;

        void SetRenderTargets(festd::span<const Core::RenderTargetHandle> renderTargets,
                              Core::RenderTargetHandle depthStencil) final;

        void SetViewportAndScissor(const Aabb& viewport, RectInt scissor) final;

        void Draw(const Core::DrawList& drawList) final;

    protected:
        explicit FrameGraphContext(Core::FrameGraph* frameGraph);

        virtual void DrawImpl(const Core::DrawList& drawList) = 0;

        struct ViewportScissorState final
        {
            Aabb m_viewport = Aabb::Initial();
            RectInt m_scissor = RectInt::Initial();
            bool m_dirty = true;
        };

        struct RenderTargetState final
        {
            Core::RenderTargetLoadOperations m_loadOperations = {};
            Core::RenderTargetStoreOperations m_storeOperations = {};
            Core::RenderTargetHandle m_renderTargets[Core::Limits::Pipeline::kMaxColorAttachments] = {};
            Core::RenderTargetHandle m_depthStencil;
            uint32_t m_renderTargetCount = 0;
        };

        std::pmr::memory_resource* m_linearAllocator = nullptr;

        PipelineStateFlags m_setStateMask = PipelineStateFlags::kNone;

        ViewportScissorState m_viewportScissorState;
        RenderTargetState m_renderTargetState;

        std::byte m_rootConstants[Core::Limits::Pipeline::kMaxRootConstantsByteSize];
        uint32_t m_rootConstantsSize = 0;

        SegmentedVector<Core::FenceSyncPoint, 256> m_signalFences;
        SegmentedVector<Core::FenceSyncPoint, 256> m_waitFences;
    };
} // namespace FE::Graphics::Common
