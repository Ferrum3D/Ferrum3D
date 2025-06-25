#pragma once
#include <FeCore/Math/Aabb.h>
#include <FeCore/Math/Colors.h>
#include <Graphics/Core/ComputePipeline.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/Fence.h>
#include <Graphics/Core/FrameGraph/Base.h>
#include <Graphics/Core/GeometryView.h>

namespace FE::Graphics::Core
{
    struct RenderTargetLoadOperations final
    {
        static constexpr uint32_t kMaxRenderTargets = 8;

        uint32_t m_colorClearMask : kMaxRenderTargets;
        uint32_t m_colorDiscardMask : kMaxRenderTargets;
        uint32_t m_depthStencilCleared : 1;
        uint32_t m_depthStencilDiscarded : 1;
        uint32_t m_unused : 6;
        uint32_t m_stencilClearValue : kMaxRenderTargets;
        float m_depthClearValue;
        Color4F m_colorClearValues[kMaxRenderTargets];

        RenderTargetLoadOperations()
        {
            memset(this, 0, sizeof(*this));
        }

        RenderTargetLoadOperations& ClearColor(const uint32_t renderTargetIndex, const Color4F color)
        {
            FE_AssertDebug(renderTargetIndex < kMaxRenderTargets, "Render target index out of range");
            FE_AssertDebug((m_colorClearMask & (1 << renderTargetIndex)) == 0, "Render target already cleared");
            FE_AssertDebug((m_colorDiscardMask & (1 << renderTargetIndex)) == 0, "Render target already discarded");

            m_colorClearValues[renderTargetIndex] = color;
            m_colorClearMask |= (1 << renderTargetIndex);
            return *this;
        }

        RenderTargetLoadOperations& ClearDepthStencil(const float depth, const uint8_t stencil)
        {
            FE_AssertDebug(m_depthStencilCleared == 0, "Depth stencil already cleared");
            FE_AssertDebug(m_depthStencilDiscarded == 0, "Depth stencil already discarded");

            m_depthClearValue = depth;
            m_stencilClearValue = stencil;
            m_depthStencilCleared = 1;
            return *this;
        }

        RenderTargetLoadOperations& DiscardColor(const uint32_t renderTargetIndex)
        {
            FE_AssertDebug(renderTargetIndex < kMaxRenderTargets, "Render target index out of range");
            FE_AssertDebug((m_colorClearMask & (1 << renderTargetIndex)) == 0, "Render target already cleared");
            FE_AssertDebug((m_colorDiscardMask & (1 << renderTargetIndex)) == 0, "Render target already discarded");

            m_colorDiscardMask |= (1 << renderTargetIndex);
            return *this;
        }

        RenderTargetLoadOperations& DiscardDepthStencil()
        {
            FE_AssertDebug(m_depthStencilCleared == 0, "Depth stencil already cleared");
            FE_AssertDebug(m_depthStencilDiscarded == 0, "Depth stencil already discarded");

            m_depthStencilDiscarded = 1;
            return *this;
        }

        RenderTargetLoadOperations& ClearAll(const Color4F color, const float depth, const uint8_t stencil)
        {
            FE_AssertDebug(m_colorClearMask == 0, "Color already cleared");
            FE_AssertDebug(m_colorDiscardMask == 0, "Color already discarded");
            FE_AssertDebug(m_depthStencilCleared == 0, "Depth stencil already cleared");
            FE_AssertDebug(m_depthStencilDiscarded == 0, "Depth stencil already discarded");

            eastl::fill_n(m_colorClearValues, festd::size(m_colorClearValues), color);

            m_depthClearValue = depth;
            m_stencilClearValue = stencil;

            m_colorClearMask = (1 << kMaxRenderTargets) - 1;
            m_depthStencilCleared = 1;
            return *this;
        }

        RenderTargetLoadOperations& DiscardAll()
        {
            FE_AssertDebug(m_colorClearMask == 0, "Color already cleared");
            FE_AssertDebug(m_colorDiscardMask == 0, "Color already discarded");
            FE_AssertDebug(m_depthStencilCleared == 0, "Depth stencil already cleared");
            FE_AssertDebug(m_depthStencilDiscarded == 0, "Depth stencil already discarded");

            m_colorDiscardMask = (1 << kMaxRenderTargets) - 1;
            m_depthStencilDiscarded = 1;
            return *this;
        }

        const static RenderTargetLoadOperations kDefault;
    };

    inline const RenderTargetLoadOperations RenderTargetLoadOperations::kDefault = {};


    struct RenderTargetStoreOperations final
    {
        static constexpr uint32_t kMaxRenderTargets = 8;

        uint32_t m_colorDiscardMask : kMaxRenderTargets;
        uint32_t m_depthStencilDiscarded : 1;
        uint32_t m_unused : 23;

        RenderTargetStoreOperations()
        {
            memset(this, 0, sizeof(*this));
        }

        RenderTargetStoreOperations& DiscardColor(const uint32_t renderTargetIndex)
        {
            FE_AssertDebug(renderTargetIndex < kMaxRenderTargets, "Render target index out of range");
            FE_AssertDebug((m_colorDiscardMask & (1 << renderTargetIndex)) == 0, "Render target already discarded");

            m_colorDiscardMask |= (1 << renderTargetIndex);
            return *this;
        }

        RenderTargetStoreOperations& DiscardDepthStencil()
        {
            FE_AssertDebug(m_depthStencilDiscarded == 0, "Depth stencil already discarded");

            m_depthStencilDiscarded = 1;
            return *this;
        }

        RenderTargetStoreOperations& DiscardAll()
        {
            FE_AssertDebug(m_colorDiscardMask == 0, "Color already discarded");
            FE_AssertDebug(m_depthStencilDiscarded == 0, "Depth stencil already discarded");

            m_colorDiscardMask = (1 << kMaxRenderTargets) - 1;
            m_depthStencilDiscarded = 1;
            return *this;
        }

        const static RenderTargetStoreOperations kDefault;
    };

    inline const RenderTargetStoreOperations RenderTargetStoreOperations::kDefault = {};


    struct FrameGraphContext : public DeviceObject
    {
        FE_RTTI_Class(FrameGraphContext, "261C8B48-9A5F-481A-B31C-AA7D48BC0E33");

        FrameGraph& GetGraph() const
        {
            return *m_frameGraph;
        }

        virtual void EnqueueFenceToWait(const FenceSyncPoint& fence) = 0;

        virtual void EnqueueFenceToSignal(const FenceSyncPoint& fence) = 0;

        virtual void PushConstants(const void* data, uint32_t size) = 0;

        template<class T>
        void PushConstants(const T& data)
        {
            PushConstants(&data, sizeof(T));
        }

        virtual void SetRenderTargetLoadOperations(const RenderTargetLoadOperations& operations) = 0;

        virtual void SetRenderTargetStoreOperations(const RenderTargetStoreOperations& operations) = 0;

        virtual void SetRenderTargets(festd::span<const RenderTargetHandle> renderTargets, RenderTargetHandle depthStencil) = 0;

        void SetRenderTarget(const RenderTargetHandle renderTarget, const RenderTargetHandle depthStencilTarget)
        {
            SetRenderTargets({ &renderTarget, 1 }, depthStencilTarget);
        }

        void SetRenderTarget(const RenderTargetHandle renderTarget)
        {
            SetRenderTargets({ &renderTarget, 1 }, RenderTargetHandle::kInvalid);
        }

        virtual void SetViewportAndScissor(RectF viewport, RectInt scissor) = 0;

        void SetViewport(const RectF viewport)
        {
            SetViewportAndScissor(viewport, RectInt(viewport));
        }

        void SetViewport(const Vector2UInt size)
        {
            const RectF viewport = RectF::FromPosAndSize({ 0.0f, 0.0f }, Vector2(size));
            const RectInt scissor = RectInt::FromPosAndSize({ 0, 0 }, Vector2Int(size));
            SetViewportAndScissor(viewport, scissor);
        }

        virtual void Draw(const DrawCall& drawCall) = 0;

        void Draw(const GraphicsPipeline* pipeline, const uint32_t vertexCount, const uint32_t instanceCount = 1,
                  const uint32_t vertexOffset = 0, const uint32_t instanceOffset = 0)
        {
            DrawArgumentsLinear drawArguments;
            drawArguments.m_vertexOffset = vertexOffset;
            drawArguments.m_vertexCount = vertexCount;

            GeometryView geometryView = {};
            geometryView.m_drawArguments.Init(drawArguments);

            DrawCall drawCall;
            drawCall.m_instanceOffset = instanceOffset;
            drawCall.m_instanceCount = instanceCount;
            drawCall.m_stencilRef = 0;
            drawCall.m_geometryView = geometryView;
            drawCall.m_pipeline = pipeline;

            Draw(drawCall);
        }

        virtual void DispatchMesh(const GraphicsPipeline* pipeline, Vector3UInt workGroupCount, uint32_t stencilRef) = 0;

        void DispatchMesh(const GraphicsPipeline* pipeline, const Vector3UInt workGroupCount)
        {
            DispatchMesh(pipeline, workGroupCount, 0);
        }

        void DispatchMesh(const GraphicsPipeline* pipeline, const Vector2UInt workGroupCount, const uint32_t stencilRef = 0)
        {
            DispatchMesh(pipeline, { workGroupCount.x, workGroupCount.y, 1 }, stencilRef);
        }

        void DispatchMesh(const GraphicsPipeline* pipeline, const uint32_t workGroupCount, const uint32_t stencilRef = 0)
        {
            DispatchMesh(pipeline, { workGroupCount, 1, 1 }, stencilRef);
        }

        virtual void Dispatch(const ComputePipeline* pipeline, Vector3UInt workGroupCount) = 0;

        void Dispatch(const ComputePipeline* pipeline, const Vector2UInt workGroupCount)
        {
            Dispatch(pipeline, { workGroupCount.x, workGroupCount.y, 1 });
        }

        void Dispatch(const ComputePipeline* pipeline, const uint32_t workGroupCount)
        {
            Dispatch(pipeline, { workGroupCount, 1, 1 });
        }

    protected:
        FrameGraph* m_frameGraph = nullptr;
    };
} // namespace FE::Graphics::Core
