#pragma once
#include <Shaders/Base/Base.h>
#include <Shaders/Scene/ViewData.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    //! Each clip level is a virtual 4096x4096 shadow map split into pages; requested pages are backed by atlas pages.
    FE_CONSTEXPR uint32_t kSunShadowMaxLevelCount = 16;
    FE_CONSTEXPR uint32_t kSunShadowPageSize = 128;
    FE_CONSTEXPR uint32_t kSunShadowLevelPagesPerAxis = 32;
    FE_CONSTEXPR uint32_t kSunShadowLevelPageCount = kSunShadowLevelPagesPerAxis * kSunShadowLevelPagesPerAxis;
    FE_CONSTEXPR uint32_t kSunShadowVirtualSize = kSunShadowPageSize * kSunShadowLevelPagesPerAxis;
    FE_CONSTEXPR uint32_t kSunShadowAtlasPagesPerAxis = 32;
    FE_CONSTEXPR uint32_t kSunShadowPhysicalPageCount = kSunShadowAtlasPagesPerAxis * kSunShadowAtlasPagesPerAxis;
    FE_CONSTEXPR uint32_t kSunShadowAtlasSize = kSunShadowPageSize * kSunShadowAtlasPagesPerAxis;

    //! Page table entries: requested by a visible pixel, and backed by the physical page in the low bits.
    FE_CONSTEXPR uint32_t kSunShadowPageRequested = 1u << 30;
    FE_CONSTEXPR uint32_t kSunShadowPageAllocated = 1u << 31;
    FE_CONSTEXPR uint32_t kSunShadowPhysicalPageMask = 0xffffu;

    FE_CONSTEXPR uint32_t kSunShadowAllocatedCounter = 0;
    FE_CONSTEXPR uint32_t kSunShadowOverflowCounter = 1;
    FE_CONSTEXPR uint32_t kSunShadowCounterCount = 4;

    FE_CONSTEXPR uint32_t kSunShadowThreadCount = 256;
    FE_CONSTEXPR uint32_t kSunShadowTileSize = 8;

    FE_CONSTEXPR uint32_t kSunShadowDebugLevel = 0;
    FE_CONSTEXPR uint32_t kSunShadowDebugPage = 1;
    FE_CONSTEXPR uint32_t kSunShadowDebugFactor = 2;


    struct SunShadowLevel final
    {
        float4x4 m_viewProjection;
        //! x: world size of a texel.
        float4 m_texelSize;
    };


    struct SunShadowData final
    {
        SunShadowLevel m_levels[kSunShadowMaxLevelCount];
        //! xyz: position of the camera that requested pages; w: world size of one of its pixels at unit distance.
        float4 m_cameraPosition;
        uint32_t m_levelCount;
        uint32_t m_isOrthographicCamera;
        float m_level0TexelSize;
        //! Clip-space depth per world unit along the light direction.
        float m_depthPerUnit;
    };


    //! Sun shadow inputs of shading passes; an invalid m_data leaves the sun unshadowed.
    struct SunShadowBinding final
    {
        StructuredBufferDescriptor<SunShadowData> m_data;
        StructuredBufferDescriptor<uint32_t> m_pageTable;
        Texture2DDescriptor<uint32_t> m_atlas;

        FE_RTTI_Reflect("9986FA74-4758-47DA-AB81-3A6953060564");
    };


    struct SunShadowClearConstants final
    {
        RWStructuredBufferDescriptor<uint32_t> m_pageTable;
        RWStructuredBufferDescriptor<uint32_t> m_counters;
        uint32_t m_pageCount;

        FE_RTTI_Reflect("28CB9DB3-34D8-4ACF-8D95-622B4FB7E51D");
    };


    struct SunShadowClearPassDesc final
    {
        SunShadowClearConstants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("71C23B37-0AD2-4648-B1C0-877EEFFF8DAC");
    };


    struct SunShadowMarkConstants final
    {
        StructuredBufferDescriptor<SunShadowData> m_data;
        StructuredBufferDescriptor<ViewData> m_view;
        Texture2DDescriptor<float> m_depth;
        RWStructuredBufferDescriptor<uint32_t> m_pageTable;
        uint2 m_resolution;

        FE_RTTI_Reflect("E507BE80-30F5-4F93-9007-4765C56FEFEC");
    };


    struct SunShadowMarkPassDesc final
    {
        SunShadowMarkConstants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("39B4514B-8578-45E2-9956-842A912A5492");
    };


    struct SunShadowAllocateConstants final
    {
        RWStructuredBufferDescriptor<uint32_t> m_pageTable;
        RWStructuredBufferDescriptor<uint32_t> m_counters;
        uint32_t m_pageCount;

        FE_RTTI_Reflect("473BC936-CCD3-46BE-A165-BB6B8FA356BE");
    };


    struct SunShadowAllocatePassDesc final
    {
        SunShadowAllocateConstants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("ADD73EEE-AE3C-4F58-AB51-0D4A1E02EF7B");
    };


    struct SunShadowClearAtlasConstants final
    {
        StructuredBufferDescriptor<uint32_t> m_counters;
        RWTexture2DDescriptor<uint32_t> m_atlas;

        FE_RTTI_Reflect("1872DD60-1782-4F94-BB67-43E9B7D4A097");
    };


    struct SunShadowClearAtlasPassDesc final
    {
        SunShadowClearAtlasConstants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("6C9D3917-12BA-43F5-A35A-CAF959977CDD");
    };


    //! Read by the shadow depth pixel shader through MeshViewData::m_passData.
    struct SunShadowRasterData final
    {
        StructuredBufferDescriptor<uint32_t> m_pageTable;
        RWTexture2DDescriptor<uint32_t> m_atlas;
        uint32_t m_level;
        uint32_t m_padding;
    };


    //! Base pass of the shadow mesh passes of one clip level; pixel shaders write the atlas without render targets.
    struct SunShadowRasterPassDesc final
    {
        //! Left invalid: it declares the render target state of a pass without attachments.
        Core::PassDepthTarget m_depthTarget;
        Core::PassViewport m_viewport;
        Core::PassTextureAccess m_atlas;

        FE_RTTI_Reflect("A3E1523A-9A5E-43EF-959C-ED7B9D428905");
    };


    struct SunShadowDebugConstants final
    {
        SunShadowBinding m_shadow;
        StructuredBufferDescriptor<ViewData> m_view;
        Texture2DDescriptor<float> m_depth;
        Texture2DDescriptor<uint32_t> m_g1;
        RWTexture2DDescriptor<float3> m_output;
        uint2 m_resolution;
        uint32_t m_mode;

        FE_RTTI_Reflect("B505A020-55DF-4B11-94EC-974A31F91BE4");
    };


    struct SunShadowDebugPassDesc final
    {
        SunShadowDebugConstants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("37935C6A-33F7-4E65-AD33-0A9CC1828412");
    };

#if FE_HOST
    static_assert(sizeof(SunShadowLevel) == 80);
    static_assert(sizeof(SunShadowData) == kSunShadowMaxLevelCount * sizeof(SunShadowLevel) + 32);
#endif
FE_HOST_END_NAMESPACE
