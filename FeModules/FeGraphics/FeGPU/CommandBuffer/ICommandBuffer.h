#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Memory/Object.h>
#include <FeGPU/Resource/ResourceState.h>
#include <cstdint>

namespace FE::GPU
{
    struct Viewport
    {
        Float32 MinX;
        Float32 MinY;
        Float32 MinZ;
        Float32 MaxX;
        Float32 MaxY;
        Float32 MaxZ;

        FE_STRUCT_RTTI(Viewport, "0BD79E66-6539-4AD3-A6DC-66066B56C5BF");

        inline Float32 Width() const noexcept
        {
            return MaxX - MinX;
        }

        inline Float32 Height() const noexcept
        {
            return MaxY - MinY;
        }

        inline Float32 Depth() const noexcept
        {
            return MaxZ - MinZ;
        }
    };

    struct Scissor
    {
        Int32 MinX;
        Int32 MinY;
        Int32 MaxX;
        Int32 MaxY;

        FE_STRUCT_RTTI(Scissor, "BC7244C7-821B-4044-B408-219A2BE1A955");

        inline Int32 Width() const noexcept
        {
            return MaxX - MinX;
        }

        inline Int32 Height() const noexcept
        {
            return MaxY - MinY;
        }
    };

    class ICommandBuffer : public IObject
    {
    public:
        virtual ~ICommandBuffer() = default;

        FE_CLASS_RTTI(ICommandBuffer, "80A845FD-5E8F-4BF1-BB75-880DE377D4A2");

        virtual void Begin() = 0;
        virtual void End()   = 0;

        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetScissor(const Scissor& scissor)    = 0;

        virtual void ResourceTransitionBarriers(const Vector<ResourceTransitionBarrierDesc>& barriers) = 0;
        virtual void MemoryBarrier()                                                                   = 0;

        virtual void Draw(UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance) = 0;
        virtual void DrawIndexed(
            UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance) = 0;
    };
} // namespace FE::GPU
