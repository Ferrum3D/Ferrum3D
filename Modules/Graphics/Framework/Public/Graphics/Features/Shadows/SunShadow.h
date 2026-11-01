#pragma once
#include <Core/Math/Transform.h>
#include <Core/Memory/Memory.h>
#include <Graphics/Base/DrawTag.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Features/Shadows/LocalShadows.h>
#include <Graphics/Passes/Tools/DepthPyramid.h>
#include <Graphics/Scene/Culling.h>
#include <Graphics/Scene/View.h>
#include <Shaders/Shadows/SunShadow.h>
#include <festd/vector.h>

namespace FE::Graphics::SunShadow
{
    struct Settings final
    {
        //! Clip levels centred on the camera; level i covers m_level0Size * 2^i.
        uint32_t m_levelCount = 8;
        //! Width of the finest clip level in world units.
        float m_level0Size = 16.0f;
        //! Distance along the light that each level covers, centred on the camera.
        float m_depthRange = 1000.0f;

        [[nodiscard]] bool Validate() const;
    };


    //! Camera and projection of one clip level, snapped to whole pages in light space so that cached pages stay valid.
    struct LevelSetup final
    {
        Transform m_camera{ kForceInit };
        Matrix4x4 m_viewProjection = Matrix4x4::kIdentity;
        float m_size = 0.0f;
        float m_texelSize = 0.0f;
        //! Light-space page of the level centre; the level covers 16 pages on each side of it.
        Vector2Int m_pageOrigin = Vector2Int::kZero;
        //! Light-space depth of the near plane, shared by all levels and snapped to a quarter of the depth range.
        float m_depthOrigin = 0.0f;
    };


    //! Clip level views around the camera position for a light that shines towards -directionToLight.
    void CalculateLevels(const Settings& settings, Vector3 cameraPosition, Vector3 directionToLight,
                         festd::fixed_vector<LevelSetup, kSunShadowMaxLevelCount>& levels);


    //! The camera whose pixels request pages; pixelFootprint is the world size of a pixel at unit distance.
    struct CameraDesc final
    {
        Vector3 m_position = Vector3::kZero;
        float m_pixelFootprint = 0.0f;
        bool m_isOrthographic = false;
    };


    [[nodiscard]] SunShadowData BuildShadowData(const Settings& settings, festd::span<const LevelSetup> levels,
                                                const CameraDesc& camera);


    struct PageCache;


    //! Static casters are drawn into cached pages when the pages are backed; dynamic casters are drawn every frame.
    enum class Layer : uint32_t
    {
        kStatic,
        kDynamic,
    };

    constexpr uint32_t kLayerCount = 2;


    //! Publishes the shadows of a camera view: the clip levels of the sun and the maps of shadow casting local lights, which
    //! share one page table and atlas. Owns the child views that render the levels and the mips of the maps.
    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("8AE8910B-A0DF-415C-A6E3-2CC1901D5A3B");

        Settings m_settings;

        explicit ViewModule(View* view);
        ~ViewModule() override;

        void Update(Core::FrameGraphBlackboard& blackboard) override;

        [[nodiscard]] festd::span<const Env::Name> GetDebugModes() const override;
        void SetDebugMode(uint32_t modeIndex) override;

        [[nodiscard]] uint32_t GetDebugMode() const override
        {
            return m_debugMode;
        }

        [[nodiscard]] View* GetLevelView(const uint32_t levelIndex, const Layer layer) const
        {
            return m_levelViews[festd::to_underlying(layer)][levelIndex].Get();
        }

        //! Culling counters of a layer summed over the clip levels; invalid until every level has read back its counters.
        [[nodiscard]] Culling::Stats GetCullingStats(Layer layer) const;

        //! The child view of one mip of a local map, created on first use.
        [[nodiscard]] View* GetLocalView(uint32_t mapIndex, uint32_t mip, Layer layer);

        [[nodiscard]] PageCache& GetPageCache() const
        {
            return *m_pageCache;
        }

        [[nodiscard]] LocalShadows::MapAllocation& GetLocalMaps()
        {
            return m_localMaps;
        }

    private:
        void DoRelease() override;

        festd::fixed_vector<Rc<View>, kSunShadowMaxLevelCount> m_levelViews[kLayerCount];
        festd::array<Rc<View>, kLocalShadowMaxMapCount * kLocalShadowMipCount> m_localViews[kLayerCount];
        festd::unique_ptr<PageCache> m_pageCache;
        LocalShadows::MapAllocation m_localMaps;
        uint32_t m_debugMode = kInvalidIndex;
    };


    //! Resources behind the SunShadowBinding of a camera view, for passes that read the binding through a buffer and declare
    //! their accesses themselves.
    struct Output final
    {
        Rc<Core::Buffer> m_data;
        Rc<Core::Buffer> m_pageTable;
        Rc<Core::Texture> m_atlas;
        //! Null when no local light casts shadows.
        Rc<Core::Buffer> m_localMaps;
        Rc<Core::Buffer> m_localLightMaps;
    };


    //! Mark, allocate and render the shadow pages of the sun and of local lights that the camera view needs; requires its
    //! main depth.
    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);

    //! Replace the view color with the selected sun shadow visualization.
    void AddDebugPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::SunShadow


namespace FE::Graphics::ShadowAtlas
{
    //! Frame inputs of one clip level, set by the camera view before its passes are added.
    struct LevelInputs final
    {
        Core::Buffer* m_pageTable = nullptr;
        Core::Texture* m_atlas = nullptr;
        //! Culls casters that touch no page drawn this frame; null disables occlusion culling.
        const Tools::DepthPyramid::Output* m_pagePyramid = nullptr;
        //! Casters drawn by the level, and the page table flags of the pages they are drawn into.
        DrawTag m_drawTag;
        uint32_t m_requiredPageFlags = 0;
        Vector2Int m_pageOrigin = Vector2Int::kZero;
        uint32_t m_level = 0;
    };


    //! Renders the shadow casters of one clip level view into the pages of the shadow atlas.
    struct ViewModule final : public ViewModuleBase
    {
        FE_RTTI("BBB80D3B-552A-4F97-A025-7349DFD49597");

        LevelInputs m_inputs;

        explicit ViewModule(View* view);
        ~ViewModule() override;

        void Update(Core::FrameGraphBlackboard& blackboard) override;

        //! Culling counters of the clip level, read back a few frames later.
        [[nodiscard]] const Culling::Stats& GetStats() const
        {
            return m_stats;
        }

    private:
        void DoRelease() override;

        Culling::Stats m_stats;
    };


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::ShadowAtlas
