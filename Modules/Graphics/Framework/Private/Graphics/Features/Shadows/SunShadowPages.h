#pragma once
#include <Core/Math/Aabb.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Fence.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Texture.h>
#include <Graphics/Features/Shadows/SunShadow.h>
#include <Graphics/Passes/Tools/DepthPyramid.h>
#include <Shaders/Shadows/SunShadow.h>
#include <festd/vector.h>

namespace FE::Graphics::SunShadow
{
    //! The page table has the blocks of the clip levels followed by those of the local shadow maps.
    [[nodiscard]] inline uint32_t GetBlockCount(const uint32_t levelCount)
    {
        return levelCount + kLocalShadowMaxMapCount * kLocalShadowMipCount;
    }


    //! Blocks reset within this many frames draw their casters before their page counts are read back.
    constexpr uint32_t kBlockFeedbackLatency = 4;


    struct FeedbackReadback final
    {
        Rc<Core::Buffer> m_buffer;
        Core::FenceSyncPoint m_fence;
        bool m_isPending = false;
    };


    //! Page table, atlas page owners and static atlas of one camera view, kept across frames so that pages whose static
    //! casters did not change are not drawn again. The clip levels of the sun and the maps of local lights share them.
    struct PageCache final
    {
        Rc<Core::Buffer> m_pageTable;
        Rc<Core::Buffer> m_owners;
        Rc<Core::Texture> m_staticAtlas;
        uint32_t m_levelCount = 0;

        //! The state the cached pages were drawn for; a change of the light or of the level layout discards them.
        Settings m_settings;
        Vector3 m_directionToLight = Vector3::kZero;
        float m_depthOrigin = 0.0f;
        bool m_hasSun = false;
        festd::fixed_vector<Vector2Int, kSunShadowMaxLevelCount> m_pageOrigins;

        //! Frames since each block was reset, saturated.
        festd::array<uint8_t, kSunShadowMaxBlockCount> m_blockAges{};
        //! Whether the static casters of each block were skipped in the previous frame, so that its dirty pages wait for them.
        festd::array<bool, kSunShadowMaxBlockCount> m_isStaticSkipped{};
        //! Requested and dirty pages of each block in the newest completed readback.
        festd::array<uint32_t, kSunShadowMaxBlockCount> m_requestedPages{};
        festd::array<uint32_t, kSunShadowMaxBlockCount> m_dirtyPages{};
        bool m_hasFeedback = false;
        festd::array<FeedbackReadback, 4> m_readbacks;
    };


    //! Frame-local resources of the page passes.
    struct PageFrame final
    {
        Rc<Core::Buffer> m_counters;
        Rc<Core::Buffer> m_freePages;
        Rc<Core::Buffer> m_evictablePages;
        Rc<Core::Buffer> m_dirtyPages;
        //! The static atlas pages of requested entries with this frame's dynamic casters added.
        Rc<Core::Texture> m_atlas;
        //! Per block, pyramids of the pages whose static casters are drawn (dirty) and of the pages sampled this frame.
        //! Blocks unused this frame have empty pyramids.
        festd::fixed_vector<Tools::DepthPyramid::Output, kSunShadowMaxBlockCount> m_staticPyramids;
        festd::fixed_vector<Tools::DepthPyramid::Output, kSunShadowMaxBlockCount> m_dynamicPyramids;
        festd::fixed_vector<SunShadowBlock, kSunShadowMaxBlockCount> m_blocks;
        //! Whether the views of each block draw their static and dynamic casters this frame. Blocks whose page counts were
        //! read back as zero have nothing to draw.
        festd::array<bool, kSunShadowMaxBlockCount> m_drawsStatic{};
        festd::array<bool, kSunShadowMaxBlockCount> m_drawsDynamic{};
        uint32_t m_pageCount = 0;
    };


    struct PageInputs final
    {
        //! A structured buffer with one SunShadowData built from m_levels.
        Core::Buffer* m_data = nullptr;
        //! A structured buffer with the ViewData of the camera whose depth requests pages.
        Core::Buffer* m_view = nullptr;
        Core::Texture* m_depth = nullptr;
        Settings m_settings;
        Vector3 m_directionToLight = Vector3::kZero;
        festd::span<const LevelSetup> m_levels;
        //! World bounds where static casters changed since the previous frame.
        festd::span<const Aabb> m_staticChanges;
        //! Without the sun, the clip levels request and draw nothing.
        bool m_hasSun = true;

        //! kLocalShadowMaxMapCount * kLocalShadowMipCount blocks of the local maps; empty when no local light casts shadows.
        festd::span<const SunShadowBlock> m_localBlocks;
        //! A bit per local map whose cached pages are discarded.
        uint32_t m_localResetMask = 0;
        //! Structured buffers of LocalShadowMapData and of the first map of every light slot.
        Core::Buffer* m_localMaps = nullptr;
        Core::Buffer* m_localLightMaps = nullptr;
        LocalLightingBinding m_localLighting;
    };


    //! Update the cache for the camera depth: release pages that scrolled out, were invalidated or belong to a different light,
    //! then request the pages of the clip levels and local maps that the depth needs. Further requests may be added before
    //! AddAllocatePasses.
    void AddRequestPasses(Core::FrameGraph& graph, Core::RingUploader& uploader, const PageInputs& inputs, PageCache& cache,
                          PageFrame& frame);

    //! Back the requested pages with free or evictable atlas pages, clear new ones in the static atlas, then build the page
    //! pyramids of the levels.
    void AddAllocatePasses(Core::FrameGraph& graph, const PageCache& cache, PageFrame& frame);

    //! Read the page counts of the blocks back, so that later frames can skip the views of blocks without pages.
    void ScheduleFeedbackReadback(Core::FrameGraph& graph, PageCache& cache, const PageFrame& frame, Core::FenceSyncPoint fence);

    //! Copy the static pages of requested entries into the frame atlas, after the static casters of dirty pages are drawn.
    void AddCopyPagesPass(Core::FrameGraph& graph, const PageCache& cache, const PageFrame& frame);
} // namespace FE::Graphics::SunShadow
