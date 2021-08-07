#pragma once
#include <FeGPU/Resource/ResourceState.h>
#include <FeCore/Memory/Memory.h>
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

        inline Int32 Width() const noexcept
        {
            return MaxX - MinX;
        }

        inline Int32 Height() const noexcept
        {
            return MaxY - MinY;
        }
    };

    class ICommandBuffer
    {
    public:
        virtual ~ICommandBuffer() = default;

        virtual void Begin() = 0;
        virtual void End()   = 0;

        virtual void SetViewport(const Viewport& viewport) = 0;
        virtual void SetScissor(const Scissor& scissor)    = 0;

        virtual void ResourceTransitionBarriers(const Vector<ResourceTransitionBarrierDesc>& barriers) = 0;
        virtual void MemoryBarrier() = 0;

        virtual void Draw(UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance) = 0;
        virtual void DrawIndexed(
            UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance) = 0;
    };
} // namespace FE::GPU
