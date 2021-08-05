#pragma once
#include <FeGPU/Resource/ResourceState.h>
#include <FeCore/Memory/Memory.h>
#include <cstdint>

namespace FE::GPU
{
    struct Viewport
    {
        float MinX;
        float MinY;
        float MinZ;
        float MaxX;
        float MaxY;
        float MaxZ;

        inline float Width() const noexcept
        {
            return MaxX - MinX;
        }

        inline float Height() const noexcept
        {
            return MaxY - MinY;
        }

        inline float Depth() const noexcept
        {
            return MaxZ - MinZ;
        }
    };

    struct Scissor
    {
        int32_t MinX;
        int32_t MinY;
        int32_t MaxX;
        int32_t MaxY;

        inline float Width() const noexcept
        {
            return MaxX - MinX;
        }

        inline float Height() const noexcept
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

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(
            uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
    };
} // namespace FE::GPU
