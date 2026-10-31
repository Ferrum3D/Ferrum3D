#pragma once
#include <Core/Math/Transform.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
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


    //! Camera and projection of one clip level, snapped to whole pages in light space.
    struct LevelSetup final
    {
        Transform m_camera{ kForceInit };
        Matrix4x4 m_viewProjection = Matrix4x4::kIdentity;
        float m_size = 0.0f;
        float m_texelSize = 0.0f;
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


    //! Publishes the sun shadows of a camera view and owns the child views that render its clip levels.
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

        [[nodiscard]] View* GetLevelView(const uint32_t levelIndex) const
        {
            return m_levelViews[levelIndex].Get();
        }

    private:
        void DoRelease() override;

        festd::fixed_vector<Rc<View>, kSunShadowMaxLevelCount> m_levelViews;
        uint32_t m_debugMode = kInvalidIndex;
    };


    //! Mark, allocate and render the shadow pages the camera view needs; requires its main depth.
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

    private:
        void DoRelease() override;
    };


    void AddPasses(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard);
} // namespace FE::Graphics::ShadowAtlas
