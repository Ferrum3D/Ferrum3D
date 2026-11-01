#pragma once
#include <Shaders/Base/Base.h>
#include <Shaders/Lighting/GlobalLighting.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    FE_CONSTEXPR uint32_t kLightClusterTileSize = 32;
    FE_CONSTEXPR uint32_t kLightClusterSlices = 24;
    FE_CONSTEXPR uint32_t kLightClusterThreadCount = 64;

    FE_CONSTEXPR float kLocalLightConeAngleScale = 65534.0f / 1.5707963267948966f;
    FE_CONSTEXPR uint32_t kLocalPointLightCone = 0xffffffffu;


    struct LocalLightData final
    {
        float4 m_positionRange;
        uint32_t m_color;
        float m_colorMultiplier;
        uint32_t m_direction;
        //! Inner/outer half-angles as two UNORM16 values; 0xffffffff denotes a point light.
        uint32_t m_coneAngles;
    };


    struct LightClusterData final
    {
        float4x4 m_view;
        float4x4 m_projection;
        float4x4 m_inverseViewProjection;
        float4 m_viewport;
        //! near, far, reciprocal logarithmic slice width, orthographic flag.
        float4 m_depth;
        uint2 m_tiles;
        uint32_t m_wordCount;
        uint32_t m_lightCount;
    };


    struct LocalLightingBinding final
    {
        BufferPointer m_lights;
        StructuredBufferDescriptor<LightClusterData> m_clusters;
        StructuredBufferDescriptor<uint32_t> m_masks;
        FE_RTTI_Reflect("B274D017-6BDB-4FFD-BB75-09DBF5AF6A18");
    };


    struct CullLocalLightsConstants final
    {
        LocalLightingBinding m_lighting;
        RWStructuredBufferDescriptor<uint32_t> m_masks;
        FE_RTTI_Reflect("24B891DF-17D1-4879-B30D-FE4415FA8458");
    };


    struct CullLocalLightsPassDesc final
    {
        CullLocalLightsConstants m_constants;
        Core::PassComputePipeline m_pipeline;
        FE_RTTI_Reflect("AF25F155-6219-43CA-98C8-5DD07BB4FF6B");
    };
FE_HOST_END_NAMESPACE
