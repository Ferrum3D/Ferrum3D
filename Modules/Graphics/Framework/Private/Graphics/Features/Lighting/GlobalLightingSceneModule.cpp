#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Features/Lighting/GlobalLightingSceneModule.h>
#include <Graphics/Features/Lighting/HosekSky.h>
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
        if (!std::isfinite(m_turbidity) || m_turbidity < 1 || m_turbidity > 10)
            return false;

        if (!IsPositive(m_sunAngularRadius) || m_sunAngularRadius >= 0.1f)
            return false;

        if (!IsNonnegative(m_groundAlbedo) || m_groundAlbedo.x > 1 || m_groundAlbedo.y > 1 || m_groundAlbedo.z > 1)
            return false;

        return true;
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
        if (m_directionalLightIlluminanceLux > 0)
            PrecomputeHosekSky(atmosphere, direction, data);

        return data;
    }


    GlobalLightingSceneModule::GlobalLightingSceneModule(Scene* scene)
        : SceneModuleBase(scene)
    {
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

        m_renderData = m_parameters.BuildRenderData();
        m_dirty = false;
    }


    void GlobalLightingSceneModule::UpdateRenderData(Core::FrameGraph& graph, Core::RingUploader&)
    {
        m_constants = graph.CreateConstants(m_renderData);
    }


    void GlobalLightingSceneModule::PrepareRenderView(Core::FrameGraph&, Core::FrameGraphBlackboard& blackboard,
                                                      Core::RingUploader&)
    {
        blackboard.Add<ConstantsPtr<GlobalLightingData>>(m_constants);
    }


    void GlobalLightingSceneModule::DoRelease()
    {
        Memory::DefaultDelete(this);
    }
} // namespace FE::Graphics
