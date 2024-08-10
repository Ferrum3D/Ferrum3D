#pragma once
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/Math/Colors.h>
#include <OsGPU/Common/Viewport.h>
#include <OsGPU/Resource/ResourceState.h>
#include <array>

namespace FE::Osmium
{
    struct ClearValueDesc
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

        FE_RTTI_Base(ClearValueDesc, "DA133DBF-F0B7-43DB-A367-C125ED14E06F");

    private:
        ClearValueDesc() = default;
    };

    struct BufferCopyRegion
    {
        uint64_t Size;
        uint32_t SourceOffset;
        uint32_t DestOffset;

        FE_RTTI_Base(BufferCopyRegion, "6D8D35DF-F12D-4F47-95EE-BA1DE3EA275E");

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

    struct BufferImageCopyRegion
    {
        Offset ImageOffset{};
        Size ImageSize{};
        ImageSubresource ImageSubresource{};
        uint64_t BufferOffset{};

        FE_RTTI_Base(BufferImageCopyRegion, "1678AA71-032A-4414-8747-F7C806FC3DC6");

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

    struct ImageBlitRegion
    {
        ImageSubresource Source;
        ImageSubresource Dest;
        Offset SourceBounds[2];
        Offset DestBounds[2];

        FE_RTTI_Base(ImageBlitRegion, "2C89E05B-1BC1-4ACE-943C-6751EA5A4B7E");
    };

    class IDescriptorTable;
    class IGraphicsPipeline;
    class IRenderPass;
    class IFramebuffer;
    class IBuffer;

    class ICommandBuffer : public Memory::RefCountedObjectBase
    {
    public:
        ~ICommandBuffer() override = default;

        FE_RTTI_Class(ICommandBuffer, "80A845FD-5E8F-4BF1-BB75-880DE377D4A2");

        virtual void Begin() = 0;
        virtual void End() = 0;

        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetScissor(const Scissor& scissor) = 0;

        virtual void ResourceTransitionBarriers(const ArraySlice<ImageBarrierDesc>& imageBarriers,
                                                const ArraySlice<BufferBarrierDesc>& bufferBarriers) = 0;
        virtual void MemoryBarrier() = 0;

        virtual void BindDescriptorTables(const ArraySlice<IDescriptorTable*>& descriptorTables, IGraphicsPipeline* pipeline) = 0;
        virtual void BindGraphicsPipeline(IGraphicsPipeline* pipeline) = 0;

        virtual void BeginRenderPass(IRenderPass* renderPass, IFramebuffer* framebuffer,
                                     const ArraySlice<ClearValueDesc>& clearValues) = 0;
        virtual void EndRenderPass() = 0;

        virtual void BindIndexBuffer(IBuffer* buffer, uint64_t byteOffset) = 0;
        virtual void BindVertexBuffer(uint32_t slot, IBuffer* buffer, uint64_t byteOffset) = 0;
        virtual void BindVertexBuffers(uint32_t startSlot, const ArraySlice<IBuffer*>& buffers,
                                       const ArraySlice<uint64_t>& offsets) = 0;

        virtual void CopyBuffers(IBuffer* source, IBuffer* dest, const BufferCopyRegion& region) = 0;
        virtual void CopyBufferToImage(IBuffer* source, IImage* dest, const BufferImageCopyRegion& region) = 0;

        virtual void BlitImage(IImage* source, IImage* dest, const ImageBlitRegion& region) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                 uint32_t firstInstance) = 0;
    };
} // namespace FE::Osmium
