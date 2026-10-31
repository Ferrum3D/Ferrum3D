#pragma once
#include <Core/Math/Aabb.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Core/Texture.h>
#include <Graphics/Features/Shadows/SunShadow.h>
#include <Graphics/Passes/Tools/DepthPyramid.h>
#include <Shaders/Shadows/SunShadow.h>
#include <festd/vector.h>

namespace FE::Graphics::SunShadow
{
    //! Page table, atlas page owners and static atlas of one camera view, kept across frames so that pages whose static
    //! casters did not change are not drawn again.
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
        festd::fixed_vector<Vector2Int, kSunShadowMaxLevelCount> m_pageOrigins;
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
        //! Per level, pyramids of the pages whose static casters are drawn (dirty) and of the pages sampled this frame.
        festd::fixed_vector<Tools::DepthPyramid::Output, kSunShadowMaxLevelCount> m_staticPyramids;
        festd::fixed_vector<Tools::DepthPyramid::Output, kSunShadowMaxLevelCount> m_dynamicPyramids;
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
    };


    //! Update the cache for the camera depth: release pages that scrolled out, were invalidated or belong to a different light,
    //! request the pages the depth needs, back new ones with free or evictable atlas pages and clear them in the static atlas,
    //! then build the page pyramids of the levels.
    void AddPagePasses(Core::FrameGraph& graph, Core::RingUploader& uploader, const PageInputs& inputs, PageCache& cache,
                       PageFrame& frame);

    //! Copy the static pages of requested entries into the frame atlas, after the static casters of dirty pages are drawn.
    void AddCopyPagesPass(Core::FrameGraph& graph, const PageCache& cache, const PageFrame& frame);
} // namespace FE::Graphics::SunShadow
