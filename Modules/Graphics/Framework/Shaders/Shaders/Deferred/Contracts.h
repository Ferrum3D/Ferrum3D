#pragma once
#include <Shaders/Passes/Visibility/Contracts.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)
    FE_CONSTEXPR uint32_t kTileSize = 128;
    // Pack several logical pixel waves into each evaluation/shading work group.
    FE_CONSTEXPR uint32_t kSurfaceWavesPerGroup = 4;
    FE_CONSTEXPR uint32_t kBackgroundShadingRoute = 0;
    FE_CONSTEXPR uint32_t kUnsupportedShadingRoute = 255;
    FE_CONSTEXPR uint32_t kExtendedPayloadValid = 1;

    struct WaveCommand final
    {
        uint32_t m_tile;
        uint32_t m_group;
        uint32_t m_pixelOffset;
        uint32_t m_validCount;
    };


    struct DispatchArguments final
    {
        uint32_t m_x;
        uint32_t m_y;
        uint32_t m_z;
    };

#if FE_HOST
    static_assert(sizeof(WaveCommand) == 16);
    static_assert(sizeof(DispatchArguments) == 12);
#endif
FE_HOST_END_NAMESPACE
