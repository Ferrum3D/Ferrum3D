#include <Framework/Entities/Query.h>
#include <GameFramework/Components/LightComponents.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Services/WorldGraphicsSceneService.h>
#include <GameFramework/Systems/LightSystem.h>
#include <cmath>

namespace FE::GameFramework
{
    bool SpotLightComponent::Validate() const
    {
        return LocalLightParameters::Validate() && std::isfinite(m_innerAngle) && std::isfinite(m_outerAngle) && m_innerAngle >= 0
            && m_outerAngle > 0 && m_innerAngle <= m_outerAngle && m_outerAngle < Constants::kPI * 0.5f;
    }


    namespace
    {
        Graphics::LocalLightData BuildLight(const Graphics::LocalLightParameters& light, const Matrix3x4& world,
                                            const float innerAngle = Constants::kPI, const float outerAngle = Constants::kPI)
        {
            Vector3 direction(0, 0, 1);
            if (outerAngle < Constants::kPI * 0.5f)
            {
                Vector3 position, scale, shear;
                Quaternion rotation;
                if (!Math::DecomposeTransform(Matrix3x4::ToMatrix4x4(world), position, rotation, scale, shear))
                    return {};

                direction = Math::Rotate(direction, rotation);
            }

            return Graphics::PackLocalLightData(world.GetTranslation(),
                                                light.m_range,
                                                Vector3(light.GetLinearColor()),
                                                light.GetCandela(innerAngle, outerAngle),
                                                direction,
                                                innerAngle,
                                                outerAngle);
        }
    } // namespace


    void LightSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<PointLightComponent>();
        world.Components().Register<SpotLightComponent>();
        m_pointChanges = {};
        m_spotChanges = {};
    }


    void LightSystem::Update(Framework::EntityUpdateContext& context)
    {
        auto* service = context.m_world.FindService<WorldGraphicsSceneService>();
        if (service == nullptr)
            return;

        auto& lights = service->GetScene().GetModules().Find<Graphics::LocalLightingSceneModule>();
        const auto point = [&lights](const PointLightComponent& light, const WorldTransformComponent& transform) {
            const Graphics::LocalLightData data =
                light.Validate() ? BuildLight(light, transform.m_world) : Graphics::LocalLightData{};
            lights.UpdateLight(light.m_light, data);
        };
        using Points = Framework::Query<const PointLightComponent, const WorldTransformComponent>;
        const auto points = Points::Create()
                                .SetChangeCursor(m_pointChanges)
                                .SetExecutionPolicy(Framework::ExecutionPolicy::kMainThread)
                                .Traverse(context, Phases::GraphicsExtraction, point);
        const auto spot = [&lights](const SpotLightComponent& light, const WorldTransformComponent& transform) {
            if (!light.Validate())
            {
                lights.UpdateLight(light.m_light, {});
                return;
            }

            const auto data = BuildLight(light, transform.m_world, light.m_innerAngle, light.m_outerAngle);
            lights.UpdateLight(light.m_light, data);
        };
        using Spots = Framework::Query<const SpotLightComponent, const WorldTransformComponent>;
        Spots::Create()
            .SetChangeCursor(m_spotChanges)
            .SetPrerequisites({ points })
            .SetExecutionPolicy(Framework::ExecutionPolicy::kMainThread)
            .Traverse(context, Phases::GraphicsExtraction, spot);
    }
} // namespace FE::GameFramework
