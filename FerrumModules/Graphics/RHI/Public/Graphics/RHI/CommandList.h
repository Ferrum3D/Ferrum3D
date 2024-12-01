#pragma once
#include <FeCore/Math/Colors.h>
#include <Graphics/RHI/Common/Viewport.h>
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/ResourceState.h>
#include <array>

namespace FE::Graphics::RHI
{
    struct ClearValueDesc final
    {
        Color4F m_colorValue;
        float m_depthValue = 0;
        uint32_t m_stencilValue = 0;
        bool m_isDepthStencil = false;

        static ClearValueDesc FE_VECTORCALL CreateColorValue(Color4F color)
        {
            ClearValueDesc result{};
            result.m_colorValue = color;
            result.m_isDepthStencil = false;
            return result;
        }

        static ClearValueDesc CreateDepthStencilValue(float depth = 1.0f, uint32_t stencil = 0)
        {
            ClearValueDesc result{};
            result.m_depthValue = depth;
            result.m_stencilValue = stencil;
            result.m_isDepthStencil = true;
            return result;
        }

    private:
        ClearValueDesc() = default;
    };

    struct BufferCopyRegion final
    {
        uint64_t m_size = 0;
        uint32_t m_sourceOffset = 0;
        uint32_t m_destOffset = 0;

        BufferCopyRegion() = default;

        explicit BufferCopyRegion(uint64_t size)
            : m_sourceOffset(0)
            , m_destOffset(0)
            , m_size(size)
        {
        }

        BufferCopyRegion(uint32_t sourceOffset, uint32_t destOffset, uint64_t size)
            : m_sourceOffset(sourceOffset)
            , m_destOffset(destOffset)
            , m_size(size)
        {
        }
    };

    struct BufferImageCopyRegion final
    {
        Offset m_imageOffset;
        Size m_imageSize;
        ImageSubresource m_imageSubresource;
        uint32_t m_bufferOffset = 0;

        BufferImageCopyRegion() = default;

        explicit BufferImageCopyRegion(Size size)
            : m_bufferOffset(0)
            , m_imageSubresource()
            , m_imageOffset()
            , m_imageSize(size)
        {
        }

        BufferImageCopyRegion(uint32_t buffetOffset, ImageSubresource imageSubresource, Offset imageOffset, Size imageSize)
            : m_bufferOffset(buffetOffset)
            , m_imageSubresource(imageSubresource)
            , m_imageOffset(imageOffset)
            , m_imageSize(imageSize)
        {
        }
    };

    struct ImageBlitRegion final
    {
        ImageSubresource m_source;
        ImageSubresource m_dest;
        Offset m_sourceBounds[2];
        Offset m_destBounds[2];
    };


    struct ShaderResourceGroup;
    struct GraphicsPipeline;
    struct RenderPass;
    struct Framebuffer;
    struct Buffer;


    enum class CommandListFlags : uint32_t
    {
        kNone = 0,
        kOneTimeSubmit = 1,
    };


    struct CommandListDesc final
    {
        HardwareQueueKindFlags m_queueKind;
        CommandListFlags m_flags;
    };


    struct CommandList : public DeviceObject
    {
        ~CommandList() override = default;

        FE_RTTI_Class(CommandList, "80A845FD-5E8F-4BF1-BB75-880DE377D4A2");

        virtual ResultCode Init(const CommandListDesc& desc) = 0;

        virtual void Begin() = 0;
        virtual void End() = 0;

        virtual void SetViewport(Viewport viewport) = 0;
        virtual void SetScissor(Scissor scissor) = 0;

        virtual void ResourceTransitionBarriers(festd::span<const ImageBarrierDesc> imageBarriers,
                                                festd::span<const BufferBarrierDesc> bufferBarriers) = 0;
        virtual void MemoryBarrier() = 0;

        virtual void BindShaderResourceGroups(festd::span<ShaderResourceGroup* const> shaderResourceGroups,
                                              GraphicsPipeline* pipeline) = 0;
        virtual void BindGraphicsPipeline(GraphicsPipeline* pipeline) = 0;

        virtual void BeginRenderPass(RenderPass* renderPass, Framebuffer* framebuffer,
                                     festd::span<const ClearValueDesc> clearValues) = 0;
        virtual void EndRenderPass() = 0;

        virtual void BindIndexBuffer(Buffer* buffer, uint64_t byteOffset) = 0;
        virtual void BindVertexBuffer(uint32_t slot, Buffer* buffer, uint64_t byteOffset) = 0;
        virtual void BindVertexBuffers(uint32_t startSlot, festd::span<Buffer* const> buffers,
                                       festd::span<const uint64_t> offsets) = 0;

        virtual void CopyBuffers(Buffer* source, Buffer* dest, const BufferCopyRegion& region) = 0;
        virtual void CopyBufferToImage(Buffer* source, Image* dest, const BufferImageCopyRegion& region) = 0;

        virtual void BlitImage(Image* source, Image* dest, const ImageBlitRegion& region) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                 uint32_t firstInstance) = 0;
    };
} // namespace FE::Graphics::RHI
