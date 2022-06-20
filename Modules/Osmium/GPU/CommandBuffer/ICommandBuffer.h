#pragma once
#include <FeCore/Containers/List.h>
#include <FeCore/Math/Color.h>
#include <FeCore/Memory/SharedPtr.h>
#include <GPU/Common/Viewport.h>
#include <GPU/Resource/ResourceState.h>

namespace FE::GPU
{
    struct ClearValueDesc
    {
        Color ColorValue;

        FE_STRUCT_RTTI(ClearValueDesc, "DA133DBF-F0B7-43DB-A367-C125ED14E06F");
    };

    struct BufferCopyRegion
    {
        UInt32 SourceOffset;
        UInt32 DestOffset;
        UInt64 Size;

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

        virtual void BeginRenderPass(IRenderPass* renderPass, IFramebuffer* framebuffer, const ClearValueDesc& clearValue) = 0;
        virtual void EndRenderPass()                                                                                       = 0;

        virtual void BindVertexBuffer(UInt32 slot, IBuffer* buffer) = 0;
        virtual void BindIndexBuffer(IBuffer* buffer)               = 0;

        virtual void CopyBuffers(IBuffer* source, IBuffer* dest, const BufferCopyRegion& region) = 0;

        virtual void Draw(UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance) = 0;
        virtual void DrawIndexed(
            UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance) = 0;
    };
} // namespace FE::GPU
