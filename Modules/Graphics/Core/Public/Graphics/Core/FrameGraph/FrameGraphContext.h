#pragma once
#include <FeCore/Math/Aabb.h>
#include <FeCore/Math/Colors.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/Texture.h>

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


    struct ComputeWorkGroupCount final
    {
        ComputeWorkGroupCount() = default;

        ComputeWorkGroupCount(const uint32_t x)
            : m_workGroupCount(x, 1, 1)
        {
        }

        ComputeWorkGroupCount(const Vector2UInt xy)
            : m_workGroupCount(xy.x, xy.y, 1)
        {
        }

        ComputeWorkGroupCount(const Vector3UInt xyz)
            : m_workGroupCount(xyz)
        {
        }

        Vector3UInt m_workGroupCount;
    };


    struct FrameGraphContext : public DeviceObject
    {
        FE_RTTI("261C8B48-9A5F-481A-B31C-AA7D48BC0E33");

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

        virtual void SetRenderTargets(festd::span<const TextureView> renderTargets, TextureView depthStencil) = 0;

        void SetRenderTarget(const TextureView renderTarget, const TextureView depthStencilTarget = TextureView::kInvalid)
        {
            SetRenderTargets({ &renderTarget, 1 }, depthStencilTarget);
        }

        virtual void SetViewport(RectF viewport) = 0;
        virtual void SetScissor(RectInt scissor) = 0;

        void SetViewport(const Vector2UInt size)
        {
            const RectF viewport = RectF::FromPosAndSize({ 0.0f, 0.0f }, Vector2(size));
            SetViewport(viewport);
        }

        virtual void SetPipeline(const GraphicsPipeline* pipeline) = 0;
        virtual void SetPipeline(const ComputePipeline* pipeline) = 0;

        virtual void SetStencilRef(uint8_t stencilRef) = 0;

        virtual void SetStreamBuffers(festd::span<const BufferView> bufferViews) = 0;
        virtual void SetIndexBuffer(BufferView bufferView, IndexType indexType) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset, uint32_t instanceOffset) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset,
                                 uint32_t instanceOffset) = 0;
        virtual void DispatchMesh(ComputeWorkGroupCount workGroupCount) = 0;
        virtual void Dispatch(ComputeWorkGroupCount workGroupCount) = 0;

        virtual bool IsCleanState() const = 0;

    protected:
        FrameGraph* m_frameGraph = nullptr;
    };
} // namespace FE::Graphics::Core
