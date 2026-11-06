#pragma once
#include <Shaders/Database/Base.h>
#include <Shaders/Lighting/GlobalLighting.h>
#include <Shaders/Lighting/LocalLightData.h>
#include <Shaders/Scene/ViewData.h>
#if FE_HOST
#    include <Graphics/Tables/Forwards.h>
#else
#    include <Shaders/Tables/LocalLightTable.hlsli>
#endif

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    FE_CONSTEXPR uint32_t kLightClusterTileSize = 32;
    FE_CONSTEXPR uint32_t kLightClusterSlices = 24;
    FE_CONSTEXPR uint32_t kLightClusterThreadCount = 64;


    struct LightClusterData final
    {
        float4x4 m_view;
        float4x4 m_projection;
        //! The camera of the clusters, for its viewport and inverse view projection.
        ConstantsPtr<ViewData> m_viewData;
        //! near, far, reciprocal logarithmic slice width, orthographic flag.
        float4 m_depth;
        uint2 m_tiles;
        uint32_t m_wordCount;
        uint32_t m_lightCount;
        FE_RTTI_Reflect("150CC6E4-302D-4C8D-9498-7A1133066AE0");
    };


    struct LocalLightingBinding final
    {
        DB::TableAddress<LocalLightTable> m_lights;
        ConstantsPtr<LightClusterData> m_clusters;
        StructuredBufferDescriptor<uint32_t> m_masks;
        FE_RTTI_Reflect("B274D017-6BDB-4FFD-BB75-09DBF5AF6A18");
    };


    struct CullLocalLightsConstants final
    {
        LocalLightingBinding m_lighting;
        RWStructuredBufferDescriptor<uint32_t> m_masks;
        FE_RTTI_Reflect("24B891DF-17D1-4879-B30D-FE4415FA8458");
    };
FE_HOST_END_NAMESPACE
