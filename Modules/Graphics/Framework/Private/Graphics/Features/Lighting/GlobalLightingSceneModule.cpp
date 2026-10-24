#include <Graphics/Features/Lighting/GlobalLightingSceneModule.h>
#include <Graphics/RendererImpl.h>
#include <cmath>

namespace FE::Graphics
{
    namespace
    {
        bool IsNonnegative(const PackedVector3F value)
        {
            return std::isfinite(value.x) && value.x >= 0 && std::isfinite(value.y) && value.y >= 0 && std::isfinite(value.z)
                && value.z >= 0;
        }


        bool IsPositive(const float value)
        {
            return std::isfinite(value) && value > 0;
        }
    } // namespace


    bool SkyAtmosphereParameters::Validate() const
    {
        if (!IsPositive(m_planetRadiusKm) || !IsPositive(m_atmosphereHeightKm))
            return false;

        if (!IsPositive(m_rayleighScaleHeightKm) || !IsPositive(m_mieScaleHeightKm) || !IsPositive(m_ozoneHalfWidthKm))
            return false;

        if (!std::isfinite(m_mieAnisotropy) || std::abs(m_mieAnisotropy) >= 0.99f)
            return false;

        if (!std::isfinite(m_mieScatteringPerKm) || m_mieScatteringPerKm < 0)
            return false;

        if (!std::isfinite(m_mieExtinctionPerKm) || m_mieExtinctionPerKm < m_mieScatteringPerKm)
            return false;

        if (!std::isfinite(m_ozoneCenterHeightKm) || m_ozoneCenterHeightKm < 0)
            return false;

        if (!IsPositive(m_sunAngularRadius) || m_sunAngularRadius >= 0.1f)
            return false;

        if (!IsNonnegative(m_groundAlbedo) || m_groundAlbedo.x > 1 || m_groundAlbedo.y > 1 || m_groundAlbedo.z > 1)
            return false;

        return IsNonnegative(m_rayleighScatteringPerKm) && IsNonnegative(m_ozoneAbsorptionPerKm);
    }


    bool GlobalLightingParameters::Validate() const
    {
        const float rotationLength = Math::LengthSquared(m_directionalLightOrientation);
        if (!std::isfinite(rotationLength) || rotationLength < 0.000001f)
            return false;

        if (!std::isfinite(m_directionalLightIlluminanceLux) || m_directionalLightIlluminanceLux < 0)
            return false;

        return IsNonnegative(m_directionalLightColor) && m_atmosphere.Validate();
    }


    GlobalLightingData GlobalLightingParameters::BuildRenderData() const
    {
        FE_Assert(Validate());

        const auto& atmosphere = m_atmosphere;
        const Vector3 direction = Math::Rotate(Vector3(0, 0, 1), Math::Normalize(m_directionalLightOrientation));
        GlobalLightingData data{};
        data.m_directionIlluminance = PackedVector4F(Vector4(direction, m_directionalLightIlluminanceLux));
        data.m_colorSunRadius = PackedVector4F(Vector4(Vector3(m_directionalLightColor), atmosphere.m_sunAngularRadius));
        data.m_rayleighScaleHeight =
            PackedVector4F(Vector4(Vector3(atmosphere.m_rayleighScatteringPerKm), atmosphere.m_rayleighScaleHeightKm));
        data.m_mieScatteringScaleHeight = PackedVector4F(atmosphere.m_mieScatteringPerKm,
                                                         atmosphere.m_mieScatteringPerKm,
                                                         atmosphere.m_mieScatteringPerKm,
                                                         atmosphere.m_mieScaleHeightKm);
        data.m_mieExtinctionAnisotropy = PackedVector4F(atmosphere.m_mieExtinctionPerKm,
                                                        atmosphere.m_mieExtinctionPerKm,
                                                        atmosphere.m_mieExtinctionPerKm,
                                                        atmosphere.m_mieAnisotropy);
        data.m_ozoneAbsorptionCenter =
            PackedVector4F(Vector4(Vector3(atmosphere.m_ozoneAbsorptionPerKm), atmosphere.m_ozoneCenterHeightKm));
        data.m_planetAtmosphereOzone =
            PackedVector4F(atmosphere.m_planetRadiusKm, atmosphere.m_atmosphereHeightKm, atmosphere.m_ozoneHalfWidthKm, 0);
        data.m_groundAlbedo = PackedVector4F(Vector4(Vector3(atmosphere.m_groundAlbedo), 0));
        return data;
    }


    GlobalLightingSceneModule::GlobalLightingSceneModule(Scene* scene)
        : SceneModuleBase(scene)
    {
        auto* renderer = Rtti::AssertCast<RendererImpl*>(scene->GetRenderer());
        m_table = Memory::DefaultNew<GlobalLightingTable>(renderer->GetDatabase());
        m_row = m_table->AllocateRow();
    }


    GlobalLightingSceneModule::~GlobalLightingSceneModule()
    {
        m_table->Free(m_row);
    }


    bool GlobalLightingSceneModule::SetParameters(const GlobalLightingParameters& parameters)
    {
        if (!parameters.Validate())
            return false;

        m_parameters = parameters;
        m_dirty = true;
        return true;
    }


    const GlobalLightingParameters& GlobalLightingSceneModule::GetParameters() const
    {
        return m_parameters;
    }


    void GlobalLightingSceneModule::Update()
    {
        if (!m_dirty)
            return;

        m_table->WriteRow(m_row).m_lighting.Get() = m_parameters.BuildRenderData();
        m_dirty = false;
    }


    void GlobalLightingSceneModule::PrepareRenderView(Core::FrameGraph&, Core::FrameGraphBlackboard& blackboard,
                                                      Core::RingUploader&)
    {
        auto& binding = blackboard.Add<GlobalLightingBinding>();
        binding.m_table = m_table->GetDeviceAddress();
        binding.m_row = m_row.m_rowIndex;
    }


    void GlobalLightingSceneModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::Graphics
