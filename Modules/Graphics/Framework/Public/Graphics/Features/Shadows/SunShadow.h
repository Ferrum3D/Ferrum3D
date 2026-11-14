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

namespace FE::Graphics
{
    struct GlobalLightingParameters;
} // namespace FE::Graphics


namespace FE::Graphics::SunShadow
{
    struct Settings final
    {
        //! Clip levels centred on the camera; level i covers m_level0Size * 2^i.
        uint32_t m_levelCount = 8;
        //! Width of the finest clip level in world units.
        float m_level0Size = 16.0f;
        //! Distance along the light that each level covers, centred on the camera.
        float m_depthRange = 65536.0f;
        //! Error in shadow texels that the LODs of casters may have. Each level and mip selects LODs for its own resolution,
        //! so the triangles drawn into coarse pages stay proportional to their texels.
        float m_lodErrorThreshold = 2.0f;
        //! Length in shadow texels of the ray that deferred shading traces through the camera depth towards the light before
        //! the shadow lookup, which starts where the ray ends. It spans the zone where the LODs of the casters differ from
        //! the triangles the camera sees; zero disables the trace.
        float m_screenTraceTexels = 4.0f;
        //! Shadows the sun in deferred shading with ScreenSpaceShadow instead of the ray of m_screenTraceTexels, which then
        //! only limits how far from the surface the shadow lookup starts. Local lights keep their rays.
        bool m_usesScreenSpaceShadow = true;
        //! Thickness that the screen space shadow assumes for every pixel of the camera depth, as a fraction of its depth.
        float m_screenShadowThickness = 0.005f;
        //! Sharpens the transition of the screen space shadow; at least one.
        float m_screenShadowContrast = 4.0f;
        //! Debug override: request only coarse receiver coverage, ignoring fine depth requests and feedback.
        bool m_forceCoarsePages = false;

        [[nodiscard]] bool Validate() const;
    };


    //! World direction from any surface towards the sun.
    [[nodiscard]] Vector3 GetDirectionToLight(const GlobalLightingParameters& parameters);


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
                         festd::fixed_vector<LevelSetup, kSunShadowMaxLevelCount>& levels,
                         const float* previousDepthOrigin = nullptr);


    //! The camera whose pixels request pages; pixelFootprint is the world size of a pixel at unit distance.
    struct CameraDesc final
    {
        Vector3 m_position = Vector3::kZero;
        float m_pixelFootprint = 0.0f;
        bool m_isOrthographic = false;
    };


    [[nodiscard]] festd::fixed_vector<SunShadowLevel, kSunShadowMaxLevelCount> BuildShadowLevels(
        festd::span<const LevelSetup> levels);

    //! The result does not reference its levels yet: set SunShadowData::m_levels to constants made from BuildShadowLevels.
    [[nodiscard]] SunShadowData BuildShadowData(const Settings& settings, const CameraDesc& camera);


    struct PageCache;
    struct PageFrame;


    //! Static casters are drawn into cached pages when the pages are backed; dynamic casters are drawn every frame.
    enum class Layer : uint32_t
    {
        kStatic,
        kDynamic,
    };

    constexpr uint32_t kLayerCount = 2;


    //! Publishes the shadows of a camera view: the clip levels of the sun and the maps of shadow casting local lights, which
    //! share one page table and atlas. The levels and the mips of the maps that have pages to draw render together, as the
    //! views of one multi-view mesh pass per layer.
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

        //! Culling counters of a layer summed over its clip levels and local map mips, read back a few frames later.
        [[nodiscard]] Culling::Stats GetCullingStats(const Layer layer) const
        {
            return m_layerStats[festd::to_underlying(layer)];
        }

        //! Where the mesh pass of a layer publishes its counters.
        [[nodiscard]] Culling::Stats& GetLayerStats(const Layer layer)
        {
            return m_layerStats[festd::to_underlying(layer)];
        }

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

        Culling::Stats m_layerStats[kLayerCount];
        festd::unique_ptr<PageCache> m_pageCache;
        LocalShadows::MapAllocation m_localMaps;
        uint32_t m_debugMode = kInvalidIndex;
    };


    //! Resources behind the SunShadowBinding of a camera view, for passes that read the binding through a buffer and declare
    //! their accesses themselves.
    struct Output final
    {
        Core::StructuredBuffer<uint32_t> m_pageTable;
        Rc<Core::Texture> m_atlas;
        Rc<Core::Texture> m_staticAtlas;
        Core::StructuredBuffer<uint32_t> m_feedback;
        //! Null when no local light casts shadows.
        Core::StructuredBuffer<LocalShadowMapData> m_localMaps;
        Core::StructuredBuffer<uint32_t> m_localLightMaps;
    };


    //! Mark, allocate and render the shadow pages of the sun and of local lights that the camera view needs; requires its
    //! main depth.
    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);

    //! Replace the view color with the selected sun shadow visualization.
    void AddDebugPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::SunShadow
