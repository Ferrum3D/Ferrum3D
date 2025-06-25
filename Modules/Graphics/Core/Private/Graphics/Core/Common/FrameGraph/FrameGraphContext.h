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
        kPushConstants = 1 << 4,
        kAllRequiredForGraphics = kLoadOperations | kStoreOperations | kRenderTargets | kViewportScissor,
        kAll = kAllRequiredForGraphics | kPushConstants,
    };

    FE_ENUM_OPERATORS(PipelineStateFlags);


    struct FrameGraphContext : public Core::FrameGraphContext
    {
        FE_RTTI_Class(FrameGraphContext, "521A8CCE-6A61-4D51-962C-16ABAB20AE89");

        void PushConstants(const void* data, uint32_t size) override;

        void SetRenderTargetLoadOperations(const Core::RenderTargetLoadOperations& operations) final;

        void SetRenderTargetStoreOperations(const Core::RenderTargetStoreOperations& operations) final;

        void SetRenderTargets(festd::span<const Core::RenderTargetHandle> renderTargets,
                              Core::RenderTargetHandle depthStencil) final;

        void SetViewportAndScissor(RectF viewport, RectInt scissor) final;

        void Draw(const Core::DrawCall& drawCall) final;
        void DispatchMesh(const Core::GraphicsPipeline* pipeline, Vector3UInt workGroupCount, uint32_t stencilRef) override;
        void Dispatch(const Core::ComputePipeline* pipeline, Vector3UInt workGroupCount) override;

    protected:
        explicit FrameGraphContext(Core::FrameGraph* frameGraph);

        virtual void DrawImpl(const Core::DrawCall& drawCall) = 0;
        virtual void DispatchMeshImpl(const Core::GraphicsPipeline* pipeline, Vector3UInt workGroupCount,
                                      uint32_t stencilRef) = 0;
        virtual void DispatchImpl(const Core::ComputePipeline* pipeline, Vector3UInt workGroupCount) = 0;

        struct ViewportScissorState final
        {
            RectF m_viewport = RectF::Initial();
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

        std::byte m_pushConstants[Core::Limits::Pipeline::kMaxPushConstantsByteSize] = {};
        uint32_t m_pushConstantsSize = 0;
    };
} // namespace FE::Graphics::Common
