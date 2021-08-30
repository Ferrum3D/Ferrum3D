#pragma once
#include <FeCore/Memory/Object.h>

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

        Viewport() = default;

        inline Viewport(Float32 minX, Float32 maxX, Float32 minY, Float32 maxY, Float32 minZ = 0, Float32 maxZ = 1.0f)
            : MinX(minX)
            , MinY(minY)
            , MinZ(minZ)
            , MaxX(maxX)
            , MaxY(maxY)
            , MaxZ(maxZ)
        {
        }

        [[nodiscard]] inline Float32 Width() const noexcept
        {
            return MaxX - MinX;
        }

        [[nodiscard]] inline Float32 Height() const noexcept
        {
            return MaxY - MinY;
        }

        [[nodiscard]] inline Float32 Depth() const noexcept
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

        Scissor() = default;

        inline explicit Scissor(const Viewport& viewport)
            : MinX(static_cast<Int32>(viewport.MinX))
            , MinY(static_cast<Int32>(viewport.MinY))
            , MaxX(static_cast<Int32>(viewport.MaxX))
            , MaxY(static_cast<Int32>(viewport.MaxY))
        {
        }

        inline Scissor(Int32 minX, Int32 maxX, Int32 minY, Int32 maxY)
            : MinX(minX)
            , MinY(minY)
            , MaxX(maxX)
            , MaxY(maxY)
        {
        }

        FE_STRUCT_RTTI(Scissor, "BC7244C7-821B-4044-B408-219A2BE1A955");

        [[nodiscard]] inline Int32 Width() const noexcept
        {
            return MaxX - MinX;
        }

        [[nodiscard]] inline Int32 Height() const noexcept
        {
            return MaxY - MinY;
        }
    };
}
