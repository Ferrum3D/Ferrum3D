#pragma once
#include <Core/Math/Quaternion.h>
#include <Graphics/Scene/Scene.h>
#include <Graphics/Tables/GlobalLightingTable.h>

namespace FE::Graphics
{
    struct SkyAtmosphereParameters final
    {
        FE_RTTI_Reflect("60B4BD91-0A5F-4F4A-84FE-9001ABD17E4A");
        FE_RTTI_Serialize();

        FE_META(Rtti::DisplayName("Planet radius (km)"))
        float m_planetRadiusKm = 6360.0f;
        FE_META(Rtti::DisplayName("Atmosphere height (km)"))
        float m_atmosphereHeightKm = 100.0f;
        FE_META(Rtti::DisplayName("Rayleigh scattering (1/km)"))
        PackedVector3F m_rayleighScatteringPerKm{ 0.005802f, 0.013558f, 0.0331f };
        FE_META(Rtti::DisplayName("Rayleigh scale height (km)"))
        float m_rayleighScaleHeightKm = 8.0f;
        FE_META(Rtti::DisplayName("Mie scattering (1/km)"))
        float m_mieScatteringPerKm = 0.003996f;
        FE_META(Rtti::DisplayName("Mie extinction (1/km)"))
        float m_mieExtinctionPerKm = 0.00444f;
        FE_META(Rtti::DisplayName("Mie scale height (km)"))
        float m_mieScaleHeightKm = 1.2f;
        FE_META(Rtti::DisplayName("Mie anisotropy"))
        float m_mieAnisotropy = 0.8f;
        FE_META(Rtti::DisplayName("Ozone absorption (1/km)"))
        PackedVector3F m_ozoneAbsorptionPerKm{ 0.00065f, 0.001881f, 0.000085f };
        FE_META(Rtti::DisplayName("Ozone center height (km)"))
        float m_ozoneCenterHeightKm = 25.0f;
        FE_META(Rtti::DisplayName("Ozone half width (km)"))
        float m_ozoneHalfWidthKm = 15.0f;
        FE_META(Rtti::DisplayName("Ground albedo"))
        PackedVector3F m_groundAlbedo{ 0.1f, 0.1f, 0.1f };
        FE_META(Rtti::DisplayName("Sun angular radius (radians)"))
        float m_sunAngularRadius = 0.004675f;

        [[nodiscard]] bool Validate() const;
    };


    struct GlobalLightingParameters final
    {
        FE_RTTI_Reflect("4916EC59-742A-498C-99DD-DC0764001791");
        FE_RTTI_Serialize();

        //! Local +Z points towards the light source; world +Y is the atmosphere's up axis at the origin.
        FE_META(Rtti::DisplayName("Sun orientation"))
        Quaternion m_directionalLightOrientation = Quaternion::kIdentity;
        FE_META(Rtti::DisplayName("Sun color (linear RGB)"))
        PackedVector3F m_directionalLightColor{ 1.0f, 1.0f, 1.0f };
        //! Illuminance on a surface perpendicular to the light, before atmospheric attenuation.
        FE_META(Rtti::DisplayName("Sun illuminance (lux)"))
        float m_directionalLightIlluminanceLux = 0.0f;
        FE_META(Rtti::DisplayName("Sky atmosphere"))
        SkyAtmosphereParameters m_atmosphere;

        [[nodiscard]] bool Validate() const;
        [[nodiscard]] GlobalLightingData BuildRenderData() const;
    };


    struct GlobalLightingSceneModule final : SceneModuleBase
    {
        FE_RTTI("636C5C42-D046-48E5-991E-90AB330AD0D0");

        explicit GlobalLightingSceneModule(Scene* scene);
        ~GlobalLightingSceneModule() override;

        bool SetParameters(const GlobalLightingParameters& parameters);
        [[nodiscard]] const GlobalLightingParameters& GetParameters() const;

        void Update() override;
        void PrepareRenderView(Core::FrameGraph& graph, Core::FrameGraphBlackboard& blackboard,
                               Core::RingUploader& uploader) override;

    private:
        void DoRelease() override;

        GlobalLightingParameters m_parameters;
        Rc<GlobalLightingTable> m_table;
        DB::Ref<GlobalLightingTable> m_row;
        bool m_dirty = true;
    };
} // namespace FE::Graphics
