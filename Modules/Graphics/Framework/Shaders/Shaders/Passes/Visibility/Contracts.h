#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)

    FE_CONSTEXPR uint32_t kContractVersion = 1;
    FE_CONSTEXPR uint32_t kPrimitiveBits = 6;
    FE_CONSTEXPR uint32_t kMaxMeshletCount = 1u << 26;
    FE_CONSTEXPR uint32_t kMaxPrimitiveCount = 1u << kPrimitiveBits;
    FE_CONSTEXPR uint32_t kMaxInstanceRow = 0xfffffffeu;
    FE_CONSTEXPR uint32_t kTileSize = 128;
    FE_CONSTEXPR uint32_t kBackgroundShadingRoute = 0;
    FE_CONSTEXPR uint32_t kUnsupportedShadingRoute = 255;
    FE_CONSTEXPR uint32_t kExtendedPayloadValid = 1;

    struct VisibilityID final
    {
        uint32_t m_instance;
        uint32_t m_primitive;
    };


    struct SurfaceRecord final
    {
        uint32_t m_colorAndShadingRoute;
        uint32_t m_octahedralNormal;
        uint32_t m_propertiesAndFlags;
        uint32_t m_extendedPayload;
    };


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
    static_assert(Core::Limits::Mesh::kMaxMeshletPrimitiveCount == kMaxPrimitiveCount);
    static_assert(sizeof(VisibilityID) == 8);
    static_assert(sizeof(SurfaceRecord) == 16);
    static_assert(sizeof(WaveCommand) == 16);
    static_assert(sizeof(DispatchArguments) == 12);

    inline VisibilityID Encode(const uint32_t instanceRow, const uint32_t meshlet, const uint32_t primitive)
    {
        FE_Assert(instanceRow <= kMaxInstanceRow);
        FE_Assert(meshlet < kMaxMeshletCount);
        FE_Assert(primitive < kMaxPrimitiveCount);
        return { instanceRow + 1, (meshlet << kPrimitiveBits) | primitive };
    }
#endif

FE_HOST_END_NAMESPACE
