#pragma once
#include <FeCore/Containers/List.h>
#include <FeCore/Math/Colors.h>
#include <FeCore/Memory/SharedPtr.h>
#include <OsGPU/Common/Viewport.h>
#include <OsGPU/Resource/ResourceState.h>

namespace FE::Osmium
{
    struct ClearValueDesc
    {
        Color ColorValue    = Colors::Black;
        Float32 DepthValue  = 0;
        UInt32 StencilValue = 0;
        bool IsDepth        = false;

        inline static ClearValueDesc CreateColorValue(const Color& color)
        {
            ClearValueDesc result{};
            result.ColorValue = color;
            result.IsDepth    = false;
            return result;
        }

        inline static ClearValueDesc CreateDepthStencilValue(float depth = 1.0f, UInt32 stencil = 0)
        {
            ClearValueDesc result{};
            result.DepthValue   = depth;
            result.StencilValue = stencil;
            result.IsDepth      = true;
            return result;
        }

        FE_STRUCT_RTTI(ClearValueDesc, "DA133DBF-F0B7-43DB-A367-C125ED14E06F");

    private:
        ClearValueDesc() = default;
    };

    struct BufferCopyRegion
    {
        UInt64 Size;
        UInt32 SourceOffset;
        UInt32 DestOffset;

        FE_STRUCT_RTTI(BufferCopyRegion, "6D8D35DF-F12D-4F47-95EE-BA1DE3EA275E");

        inline BufferCopyRegion() = default;

        inline explicit BufferCopyRegion(UInt64 size)
            : SourceOffset(0)
            , DestOffset(0)
            , Size(size)
        {
        }

        inline BufferCopyRegion(UInt32 sourceOffset, UInt32 destOffset, UInt64 size)
            : SourceOffset(sourceOffset)
            , DestOffset(destOffset)
            , Size(size)
        {
        }
    };

    struct BufferImageCopyRegion
    {
        Offset ImageOffset{};
        Size ImageSize{};
        ImageSubresource ImageSubresource{};
        UInt64 BufferOffset{};

        FE_STRUCT_RTTI(BufferImageCopyRegion, "1678AA71-032A-4414-8747-F7C806FC3DC6");

        inline BufferImageCopyRegion() = default;

        inline explicit BufferImageCopyRegion(Size size)
            : BufferOffset(0)
            , ImageSubresource()
            , ImageOffset()
            , ImageSize(size)
        {
        }

        inline BufferImageCopyRegion(
            UInt64 buffetOffset, struct ImageSubresource imageSubresource, Offset imageOffset, Size imageSize)
            : BufferOffset(buffetOffset)
            , ImageSubresource(imageSubresource)
            , ImageOffset(imageOffset)
            , ImageSize(imageSize)
        {
        }
    };

    struct ImageBlitRegion
    {
        FE_STRUCT_RTTI(ImageBlitRegion, "2C89E05B-1BC1-4ACE-943C-6751EA5A4B7E");
        ImageSubresource Source;
        ImageSubresource Dest;
        Offset SourceBounds[2];
        Offset DestBounds[2];
    };

    class IDescriptorTable;
    class IGraphicsPipeline;
    class IRenderPass;
    class IFramebuffer;
    class IBuffer;

    class ICommandBuffer : public IObject
    {
    public:
        ~ICommandBuffer() override = default;

        FE_CLASS_RTTI(ICommandBuffer, "80A845FD-5E8F-4BF1-BB75-880DE377D4A2");

        virtual void Begin() = 0;
        virtual void End()   = 0;

        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetScissor(const Scissor& scissor)    = 0;

        virtual void ResourceTransitionBarriers(const List<ResourceTransitionBarrierDesc>& barriers) = 0;
        virtual void MemoryBarrier()                                                                 = 0;

        virtual void BindDescriptorTables(const List<IDescriptorTable*>& descriptorTables, IGraphicsPipeline* pipeline) = 0;
        virtual void BindGraphicsPipeline(IGraphicsPipeline* pipeline)                                                  = 0;

        virtual void BeginRenderPass(
            IRenderPass* renderPass, IFramebuffer* framebuffer, const List<ClearValueDesc>& clearValues) = 0;
        virtual void EndRenderPass()                                                                     = 0;

        virtual void BindVertexBuffer(UInt32 slot, IBuffer* buffer) = 0;
        virtual void BindIndexBuffer(IBuffer* buffer)               = 0;

        virtual void CopyBuffers(IBuffer* source, IBuffer* dest, const BufferCopyRegion& region)           = 0;
        virtual void CopyBufferToImage(IBuffer* source, IImage* dest, const BufferImageCopyRegion& region) = 0;

        virtual void BlitImage(IImage* source, IImage* dest, const ImageBlitRegion& region) = 0;

        virtual void Draw(UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance) = 0;
        virtual void DrawIndexed(
            UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance) = 0;
    };
} // namespace FE::Osmium
