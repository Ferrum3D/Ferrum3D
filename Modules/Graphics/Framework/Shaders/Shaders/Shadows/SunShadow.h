#pragma once
#include <Shaders/Base/Base.h>
#include <Shaders/Lighting/LocalLighting.h>
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

    //! Page table entries persist across frames. Each level addresses its 32x32 entries toroidally by world page, so a level
    //! that scrolls by whole pages keeps the entries still inside it. The low bits hold the backing atlas page.
    FE_CONSTEXPR uint32_t kSunShadowPageAllocated = 1u << 31;
    //! Requested by a visible pixel this frame.
    FE_CONSTEXPR uint32_t kSunShadowPageRequested = 1u << 30;
    //! Backed this frame, so its static casters are drawn into the static atlas.
    FE_CONSTEXPR uint32_t kSunShadowPageDirty = 1u << 29;
    FE_CONSTEXPR uint32_t kSunShadowPhysicalPageMask = 0xffffu;
    //! Owner of an atlas page that backs no page table entry.
    FE_CONSTEXPR uint32_t kSunShadowNoOwner = 0xffffffffu;

    //! Pages backed this frame, which are also the dirty pages.
    FE_CONSTEXPR uint32_t kSunShadowAllocatedCounter = 0;
    FE_CONSTEXPR uint32_t kSunShadowOverflowCounter = 1;
    //! Atlas pages without an owner, and atlas pages whose owner was not requested this frame.
    FE_CONSTEXPR uint32_t kSunShadowFreeCounter = 2;
    FE_CONSTEXPR uint32_t kSunShadowEvictableCounter = 3;
    FE_CONSTEXPR uint32_t kSunShadowFreeTakenCounter = 4;
    FE_CONSTEXPR uint32_t kSunShadowEvictableTakenCounter = 5;

    //! The page mask of a level covers each page with 2x2 texels, so the first mip of its pyramid has one texel per page.
    FE_CONSTEXPR uint32_t kSunShadowMaskTexelsPerPage = 2;
    FE_CONSTEXPR uint32_t kSunShadowMaskSize = kSunShadowLevelPagesPerAxis * kSunShadowMaskTexelsPerPage;

    FE_CONSTEXPR uint32_t kSunShadowThreadCount = 256;
    FE_CONSTEXPR uint32_t kSunShadowTileSize = 8;

    //! Local light shadow maps share the page table and the atlas with the clip levels. Their blocks follow the clip levels:
    //! each map has kLocalShadowMipCount mips of kSunShadowVirtualSize >> mip texels, and each mip takes one block of
    //! kSunShadowLevelPageCount entries, of which it uses the first (kSunShadowLevelPagesPerAxis >> mip) pages per axis.
    FE_CONSTEXPR uint32_t kLocalShadowMaxMapCount = 16;
    FE_CONSTEXPR uint32_t kLocalShadowMipCount = 4;
    FE_CONSTEXPR uint32_t kLocalShadowPointFaceCount = 6;
    FE_CONSTEXPR uint32_t kSunShadowMaxBlockCount = kSunShadowMaxLevelCount + kLocalShadowMaxMapCount * kLocalShadowMipCount;
    //! Per light slot: the first map of a shadowed light, with this flag for point lights, whose six faces follow it.
    FE_CONSTEXPR uint32_t kLocalShadowPointFlag = 1u << 31;
    FE_CONSTEXPR uint32_t kLocalShadowNoMap = 0xffffffffu;

    //! Per block, the requested pages and the dirty pages after allocation. They are read back to skip the views of blocks
    //! that have nothing to draw.
    FE_CONSTEXPR uint32_t kSunShadowBlockRequestCounter = 8;
    FE_CONSTEXPR uint32_t kSunShadowBlockDirtyCounter = kSunShadowBlockRequestCounter + kSunShadowMaxBlockCount;
    FE_CONSTEXPR uint32_t kSunShadowCounterCount = kSunShadowBlockDirtyCounter + kSunShadowMaxBlockCount;

    FE_CONSTEXPR uint32_t kSunShadowDebugLevel = 0;
    FE_CONSTEXPR uint32_t kSunShadowDebugPage = 1;
    FE_CONSTEXPR uint32_t kSunShadowDebugFactor = 2;
    //! The lowest visibility of the shadowed local lights that reach each pixel; blue where none does.
    FE_CONSTEXPR uint32_t kSunShadowDebugLocalFactor = 3;


    struct SunShadowLevel final
    {
        float4x4 m_viewProjection;
        //! World size of a texel.
        float m_texelSize;
        uint32_t m_padding;
        //! World page at the first page of the level; world page (u, v) shows at page (u - x, v + y) of the level.
        int2 m_pageOrigin;
        FE_RTTI_Reflect("3F6A9C21-7B4E-4D18-A2C6-9E1D07B5F384");
    };


    struct SunShadowData final
    {
        //! m_levelCount elements, loaded one at a time.
        ConstantsPtr<SunShadowLevel> m_levels;
        //! xyz: position of the camera that requested pages; w: world size of one of its pixels at unit distance.
        float4 m_cameraPosition;
        uint32_t m_levelCount;
        uint32_t m_isOrthographicCamera;
        float m_level0TexelSize;
        //! Clip-space depth per world unit along the light direction.
        float m_depthPerUnit;
        FE_RTTI_Reflect("28DC5AE5-CDBC-4AC3-9EFC-C6C58106A648");
    };


    //! How each block of the page table moved since the previous frame, in whole pages, and whether its pages are discarded.
    struct SunShadowScroll final
    {
        int2 m_previousOrigin;
        int2 m_currentOrigin;
        uint32_t m_isReset;
        //! The static casters of the block were not drawn in the previous frame, so its dirty pages stay dirty.
        uint32_t m_keepsDirty;
    };


    //! The projection of one block of the page table, for invalidating the pages that changed casters cover.
    struct SunShadowBlock final
    {
        float4x4 m_viewProjection;
        int2 m_pageOrigin;
        //! Zero for blocks that no level or map uses this frame.
        uint32_t m_pagesPerAxis;
        uint32_t m_isPerspective;
    };


    //! One perspective shadow map of a local light: a spot light or a cube face of a point light.
    struct LocalShadowMapData final
    {
        float4x4 m_viewProjection;
        //! xyz: light position; w: world size of a texel of mip 0 per unit of distance along m_forward.
        float4 m_positionTexelScale;
        float3 m_forward;
        uint32_t m_firstBlock;
    };


    //! Sun shadow inputs of shading passes; an invalid m_data leaves the sun unshadowed.
    struct SunShadowBinding final
    {
        ConstantsPtr<SunShadowData> m_data;
        StructuredBufferDescriptor<uint32_t> m_pageTable;
        Texture2DDescriptor<uint32_t> m_atlas;
        StructuredBufferDescriptor<LocalShadowMapData> m_localMaps;
        //! Indexed by light slot, see kLocalShadowPointFlag.
        StructuredBufferDescriptor<uint32_t> m_localLightMaps;

        FE_RTTI_Reflect("9986FA74-4758-47DA-AB81-3A6953060564");
    };


    //! Clears the per-frame page bits and frees the pages that scrolled out of their level, or every page on a reset.
    struct SunShadowUpdateConstants final
    {
        RWStructuredBufferDescriptor<uint32_t> m_pageTable;
        RWStructuredBufferDescriptor<uint32_t> m_owners;
        RWStructuredBufferDescriptor<uint32_t> m_counters;
        StructuredBufferDescriptor<SunShadowScroll> m_scroll;
        uint32_t m_pageCount;
        uint32_t m_isReset;

        FE_RTTI_Reflect("28CB9DB3-34D8-4ACF-8D95-622B4FB7E51D");
    };


    //! Frees the pages of every block that overlap world bounds where static casters changed.
    struct SunShadowInvalidateConstants final
    {
        StructuredBufferDescriptor<SunShadowBlock> m_blocks;
        //! Pairs of minimum and maximum corners.
        StructuredBufferDescriptor<float4> m_regions;
        RWStructuredBufferDescriptor<uint32_t> m_pageTable;
        RWStructuredBufferDescriptor<uint32_t> m_owners;

        FE_RTTI_Reflect("989CB824-D885-4004-A298-672C2B610EE6");
    };


    struct SunShadowMarkConstants final
    {
        ConstantsPtr<SunShadowData> m_data;
        ConstantsPtr<ViewData> m_view;
        Texture2DDescriptor<float> m_depth;
        RWStructuredBufferDescriptor<uint32_t> m_pageTable;
        LocalLightingBinding m_localLighting;
        StructuredBufferDescriptor<LocalShadowMapData> m_localMaps;
        StructuredBufferDescriptor<uint32_t> m_localLightMaps;
        uint2 m_resolution;

        FE_RTTI_Reflect("E507BE80-30F5-4F93-9007-4765C56FEFEC");
    };


    //! Lists the atlas pages that new requests may take: free ones first, then those whose page was not requested.
    struct SunShadowCollectConstants final
    {
        StructuredBufferDescriptor<uint32_t> m_pageTable;
        StructuredBufferDescriptor<uint32_t> m_owners;
        RWStructuredBufferDescriptor<uint32_t> m_freePages;
        RWStructuredBufferDescriptor<uint32_t> m_evictablePages;
        RWStructuredBufferDescriptor<uint32_t> m_counters;

        FE_RTTI_Reflect("B1F80779-5448-495F-B829-360D17BA8DA6");
    };


    struct SunShadowAllocateConstants final
    {
        RWStructuredBufferDescriptor<uint32_t> m_pageTable;
        RWStructuredBufferDescriptor<uint32_t> m_owners;
        StructuredBufferDescriptor<uint32_t> m_freePages;
        StructuredBufferDescriptor<uint32_t> m_evictablePages;
        RWStructuredBufferDescriptor<uint32_t> m_dirtyPages;
        RWStructuredBufferDescriptor<uint32_t> m_counters;
        uint32_t m_pageCount;

        FE_RTTI_Reflect("473BC936-CCD3-46BE-A165-BB6B8FA356BE");
    };


    //! Clears the static atlas pages that are drawn this frame.
    struct SunShadowClearAtlasConstants final
    {
        StructuredBufferDescriptor<uint32_t> m_counters;
        StructuredBufferDescriptor<uint32_t> m_dirtyPages;
        RWTexture2DDescriptor<uint32_t> m_atlas;

        FE_RTTI_Reflect("1872DD60-1782-4F94-BB67-43E9B7D4A097");
    };


    //! Copies the static atlas pages of requested entries into the atlas that dynamic casters are added to.
    struct SunShadowCopyPagesConstants final
    {
        StructuredBufferDescriptor<uint32_t> m_pageTable;
        StructuredBufferDescriptor<uint32_t> m_owners;
        Texture2DDescriptor<uint32_t> m_source;
        RWTexture2DDescriptor<uint32_t> m_target;

        FE_RTTI_Reflect("E0B9E7A5-DC51-4E6F-A9FF-E4D063D5B436");
    };


    //! Writes the page mask of one level: 0 (farthest) where pages with all of m_requiredFlags are drawn, 1 (nearest)
    //! elsewhere. Its farthest depth pyramid then culls casters that touch no drawn page with the regular occlusion test.
    struct SunShadowPageMaskConstants final
    {
        StructuredBufferDescriptor<uint32_t> m_pageTable;
        RWTexture2DDescriptor<float> m_mask;
        uint32_t m_level;
        uint32_t m_requiredFlags;
        int2 m_pageOrigin;
        uint32_t m_pagesPerAxis;

        FE_RTTI_Reflect("DA1D75B4-3D68-491F-B2F5-4F3ABDD108D3");
    };


    //! Read by the shadow depth pixel shader through MeshViewData::m_passData; it writes pages with all of m_requiredFlags.
    struct SunShadowRasterData final
    {
        StructuredBufferDescriptor<uint32_t> m_pageTable;
        RWTexture2DDescriptor<uint32_t> m_atlas;
        uint32_t m_level;
        uint32_t m_requiredFlags;
        int2 m_pageOrigin;
        FE_RTTI_Reflect("C9438732-66F5-451D-A219-B71D3A5FB449");
    };


    //! Base pass of the shadow mesh passes of one clip level; pixel shaders write the atlas without render targets.
    struct SunShadowRasterPassDesc final
    {
        //! Left invalid: it declares the render target state of a pass without attachments.
        Core::PassDepthTarget m_depthTarget;
        Core::PassViewport m_viewport;

        FE_RTTI_Reflect("A3E1523A-9A5E-43EF-959C-ED7B9D428905");
    };


    //! Read by the page marking pixel shader of transparent surfaces through MeshViewData::m_passData.
    struct SunShadowMarkRasterData final
    {
        ConstantsPtr<SunShadowData> m_data;
        RWStructuredBufferDescriptor<uint32_t> m_pageTable;
        LocalLightingBinding m_localLighting;
        StructuredBufferDescriptor<LocalShadowMapData> m_localMaps;
        StructuredBufferDescriptor<uint32_t> m_localLightMaps;
        FE_RTTI_Reflect("F1905EB0-46BE-4F0C-9DCE-40D78AA3F97F");
    };


    //! Base pass of the mesh passes that request pages for transparent surfaces in front of the camera depth.
    struct SunShadowMarkRasterPassDesc final
    {
        //! Tested but not written, so only surfaces in front of opaque geometry request pages.
        Core::PassDepthTarget m_depthTarget;
        Core::PassViewport m_viewport;

        FE_RTTI_Reflect("465C550C-FB30-48C4-9955-941047DDFB20");
    };


    struct SunShadowDebugConstants final
    {
        SunShadowBinding m_shadow;
        LocalLightingBinding m_localLighting;
        ConstantsPtr<ViewData> m_view;
        Texture2DDescriptor<float> m_depth;
        Texture2DDescriptor<uint32_t> m_g1;
        RWTexture2DDescriptor<float3> m_output;
        uint2 m_resolution;
        uint32_t m_mode;

        FE_RTTI_Reflect("B505A020-55DF-4B11-94EC-974A31F91BE4");
    };

#if FE_HOST
    static_assert(sizeof(SunShadowLevel) == 80);
    static_assert(sizeof(SunShadowData) == 40);
#endif
FE_HOST_END_NAMESPACE
