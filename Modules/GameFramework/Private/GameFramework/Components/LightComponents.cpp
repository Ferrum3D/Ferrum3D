#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/Components/LightComponents.h>
#include <GameFramework/Services/WorldGraphicsSceneService.h>
#include <cmath>

namespace FE::GameFramework
{
    namespace
    {
        Framework::LifecycleResult ActivateLight(Graphics::LightHandle& light, Framework::ComponentContext& context)
        {
            auto* service = context.m_world.FindService<WorldGraphicsSceneService>();
            if (service == nullptr)
                return Framework::LifecycleResult::kFailed;

            light = service->GetScene().GetModules().Find<Graphics::LocalLightingSceneModule>().CreateLight();
            return Framework::LifecycleResult::kSucceeded;
        }


        void DeactivateLight(Graphics::LightHandle& light, Framework::ComponentContext& context)
        {
            auto* service = context.m_world.FindService<WorldGraphicsSceneService>();
            FE_Assert(service);
            service->GetScene().GetModules().Find<Graphics::LocalLightingSceneModule>().DestroyLight(light);
            light = {};
        }
    } // namespace


    PointLightComponent::PointLightComponent(const PointLightComponent& other)
        : LocalLightParameters(other)
    {
    }


    Framework::LifecycleResult PointLightComponent::Activate(Framework::ComponentContext& context)
    {
        return ActivateLight(m_light, context);
    }


    void PointLightComponent::Deactivate(Framework::ComponentContext& context)
    {
        DeactivateLight(m_light, context);
    }


    SpotLightComponent::SpotLightComponent(const SpotLightComponent& other)
        : LocalLightParameters(other)
        , m_innerAngle(other.m_innerAngle)
        , m_outerAngle(other.m_outerAngle)
    {
    }


    Framework::LifecycleResult SpotLightComponent::Activate(Framework::ComponentContext& context)
    {
        return ActivateLight(m_light, context);
    }


    void SpotLightComponent::Deactivate(Framework::ComponentContext& context)
    {
        DeactivateLight(m_light, context);
    }


    bool SpotLightComponent::Validate() const
    {
        return LocalLightParameters::Validate() && std::isfinite(m_innerAngle) && std::isfinite(m_outerAngle) && m_innerAngle >= 0
            && m_outerAngle > 0 && m_innerAngle <= m_outerAngle && m_outerAngle < Constants::kPI * 0.5f;
    }
} // namespace FE::GameFramework
