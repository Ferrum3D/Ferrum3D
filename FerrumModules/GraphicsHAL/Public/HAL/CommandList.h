#pragma once
#include <FeCore/Math/Colors.h>
#include <HAL/Common/Viewport.h>
#include <HAL/DeviceObject.h>
#include <HAL/ResourceState.h>
#include <array>

namespace FE::Graphics::HAL
{
    struct ClearValueDesc final
    {
        std::array<float, 4> ColorValue;
        float DepthValue = 0;
        uint32_t StencilValue = 0;
        bool IsDepth = false;

        inline static ClearValueDesc CreateColorValue(const Color& color)
        {
            ClearValueDesc result{};
            for (size_t i = 0; i < result.ColorValue.size(); ++i)
            {
                result.ColorValue[i] = color[i];
            }
            result.IsDepth = false;
            return result;
        }

        inline static ClearValueDesc CreateDepthStencilValue(float depth = 1.0f, uint32_t stencil = 0)
        {
            ClearValueDesc result{};
            result.DepthValue = depth;
            result.StencilValue = stencil;
            result.IsDepth = true;
            return result;
        }

    private:
        ClearValueDesc() = default;
    };

    struct BufferCopyRegion final
    {
        uint64_t Size;
        uint32_t SourceOffset;
        uint32_t DestOffset;

        inline BufferCopyRegion() = default;

        inline explicit BufferCopyRegion(uint64_t size)
            : SourceOffset(0)
            , DestOffset(0)
            , Size(size)
        {
        }

        inline BufferCopyRegion(uint32_t sourceOffset, uint32_t destOffset, uint64_t size)
            : SourceOffset(sourceOffset)
            , DestOffset(destOffset)
            , Size(size)
        {
        }
    };

    struct BufferImageCopyRegion final
    {
        Offset ImageOffset{};
        Size ImageSize{};
        ImageSubresource ImageSubresource{};
        uint64_t BufferOffset{};

        inline BufferImageCopyRegion() = default;

        inline explicit BufferImageCopyRegion(Size size)
            : BufferOffset(0)
            , ImageSubresource()
            , ImageOffset()
            , ImageSize(size)
        {
        }

        inline BufferImageCopyRegion(uint64_t buffetOffset, struct ImageSubresource imageSubresource, Offset imageOffset,
                                     Size imageSize)
            : BufferOffset(buffetOffset)
            , ImageSubresource(imageSubresource)
            , ImageOffset(imageOffset)
            , ImageSize(imageSize)
        {
        }
    };

    struct ImageBlitRegion final
    {
        ImageSubresource Source;
        ImageSubresource Dest;
        Offset SourceBounds[2];
        Offset DestBounds[2];
    };


    class ShaderResourceGroup;
    class GraphicsPipeline;
    class RenderPass;
    class Framebuffer;
    class Buffer;


    enum class CommandListFlags
    {
        None = 0,
        OneTimeSubmit = 1,
    };


    struct CommandListDesc final
    {
        HardwareQueueKindFlags QueueKind;
        CommandListFlags Flags;
    };


    class CommandList : public DeviceObject
    {
    public:
        ~CommandList() override = default;

        FE_RTTI_Class(CommandList, "80A845FD-5E8F-4BF1-BB75-880DE377D4A2");

        virtual ResultCode Init(const CommandListDesc& desc) = 0;

        virtual void Begin() = 0;
        virtual void End() = 0;

        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetScissor(const Scissor& scissor) = 0;

        virtual void ResourceTransitionBarriers(festd::span<const ImageBarrierDesc> imageBarriers,
                                                festd::span<const BufferBarrierDesc> bufferBarriers) = 0;
        virtual void MemoryBarrier() = 0;

        virtual void BindShaderResourceGroups(festd::span<ShaderResourceGroup*> shaderResourceGroups,
                                              GraphicsPipeline* pipeline) = 0;
        virtual void BindGraphicsPipeline(GraphicsPipeline* pipeline) = 0;

        virtual void BeginRenderPass(RenderPass* renderPass, Framebuffer* framebuffer,
                                     const festd::span<ClearValueDesc>& clearValues) = 0;
        virtual void EndRenderPass() = 0;

        virtual void BindIndexBuffer(Buffer* buffer, uint64_t byteOffset) = 0;
        virtual void BindVertexBuffer(uint32_t slot, Buffer* buffer, uint64_t byteOffset) = 0;
        virtual void BindVertexBuffers(uint32_t startSlot, festd::span<Buffer*> buffers, festd::span<const uint64_t> offsets) = 0;

        virtual void CopyBuffers(Buffer* source, Buffer* dest, const BufferCopyRegion& region) = 0;
        virtual void CopyBufferToImage(Buffer* source, Image* dest, const BufferImageCopyRegion& region) = 0;

        virtual void BlitImage(Image* source, Image* dest, const ImageBlitRegion& region) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                 uint32_t firstInstance) = 0;
    };
} // namespace FE::Graphics::HAL
