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
        kStencilRef = 1 << 5,
        kGraphicsPipeline = 1 << 6,
        kComputePipeline = 1 << 7,
        kStreamBuffers = 1 << 8,
        kIndexBuffer = 1 << 9,
        kAllRequiredForGraphics = kLoadOperations | kStoreOperations | kRenderTargets | kGraphicsPipeline,
        kAll = kAllRequiredForGraphics | kPushConstants,
    };

    FE_ENUM_OPERATORS(PipelineStateFlags);


    struct FrameGraphContext : public Core::FrameGraphContext
    {
        FE_RTTI("521A8CCE-6A61-4D51-962C-16ABAB20AE89");

        void PushConstants(const void* data, uint32_t size) override;

        void SetRenderTargetLoadOperations(const Core::RenderTargetLoadOperations& operations) final;
        void SetRenderTargetStoreOperations(const Core::RenderTargetStoreOperations& operations) final;
        void SetRenderTargets(festd::span<const Core::TextureView> renderTargets, Core::TextureView depthStencil) final;

        void SetViewportAndScissor(RectF viewport, RectInt scissor) final;

        void SetPipeline(const Core::GraphicsPipeline* pipeline) final;
        void SetPipeline(const Core::ComputePipeline* pipeline) final;

        void SetStencilRef(uint8_t stencilRef) final;

        void SetStreamBuffers(festd::span<const Core::BufferView> bufferViews) override;
        void SetIndexBuffer(Core::BufferView bufferView, Core::IndexType indexType) override;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset, uint32_t instanceOffset) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset,
                         uint32_t instanceOffset) final;
        void DispatchMesh(Core::ComputeWorkGroupCount workGroupCount) final;
        void Dispatch(Core::ComputeWorkGroupCount workGroupCount) final;

        bool IsCleanState() const override;

    protected:
        explicit FrameGraphContext(Core::FrameGraph* frameGraph);

        virtual void DrawImpl(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset, uint32_t instanceOffset) = 0;
        virtual void DrawIndexedImpl(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset,
                                     uint32_t instanceOffset) = 0;
        virtual void DispatchMeshImpl(Vector3UInt workGroupCount) = 0;
        virtual void DispatchImpl(Vector3UInt workGroupCount) = 0;

        void ClearStatesInternal();
        void PrepareStatesInternal();

        enum class StateAction
        {
            kKeep,
            kSet,
            kReset,
        };

        struct ViewportScissorState final
        {
            RectF m_viewport = RectF::Initial();
            RectInt m_scissor = RectInt::Initial();
            StateAction m_action = StateAction::kReset;
        };

        struct RenderTargetState final
        {
            Core::RenderTargetLoadOperations m_loadOperations = {};
            Core::RenderTargetStoreOperations m_storeOperations = {};
            Core::TextureView m_renderTargets[Core::Limits::Pipeline::kMaxColorAttachments] = {};
            Core::TextureView m_depthStencil;
            uint32_t m_renderTargetCount = 0;
        };

        struct PipelineState final
        {
            const Core::GraphicsPipeline* m_graphicsPipeline = nullptr;
            const Core::ComputePipeline* m_computePipeline = nullptr;
            StateAction m_action = StateAction::kReset;
        };

        struct StencilRefState final
        {
            uint8_t m_stencilRef = 0;
            StateAction m_action = StateAction::kReset;
        };

        Core::BufferView m_streamBufferViews[Core::Limits::Pipeline::kMaxVertexStreams] = {};
        Core::BufferView m_indexBufferView = Core::BufferView::kInvalid;
        Core::IndexType m_indexType = Core::IndexType::kUint32;

        std::pmr::memory_resource* m_linearAllocator = nullptr;

        PipelineStateFlags m_setStateMask = PipelineStateFlags::kNone;

        ViewportScissorState m_viewportScissorState;
        RenderTargetState m_renderTargetState;
        PipelineState m_pipelineState;
        StencilRefState m_stencilRefState;

        std::byte m_pushConstants[Core::Limits::Pipeline::kMaxPushConstantsByteSize] = {};
        uint32_t m_pushConstantsSize = 0;
    };
} // namespace FE::Graphics::Common
