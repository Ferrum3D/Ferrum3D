#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/Components/PhysicsBodyComponent.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Services/WorldPhysicsService.h>

namespace FE::GameFramework
{
    PhysicsBodyComponent::PhysicsBodyComponent(const PhysicsBodyComponent& other)
        : m_shapes(other.m_shapes)
        , m_motionType(other.m_motionType)
        , m_mass(other.m_mass)
        , m_friction(other.m_friction)
        , m_restitution(other.m_restitution)
        , m_linearVelocity(other.m_linearVelocity)
        , m_angularVelocity(other.m_angularVelocity)
    {
    }


    Framework::LifecycleResult PhysicsBodyComponent::Activate(Framework::ComponentContext& context)
    {
        auto* physics = context.m_world.FindService<WorldPhysicsService>();
        const auto* transform = context.m_entity.FindComponent<const TransformComponent>();
        if (!physics || !transform)
            return Framework::LifecycleResult::kFailed;

        for (auto* parent = context.m_entity.GetParent(); parent; parent = parent->GetParent())
        {
            if (parent->FindComponent<const TransformComponent>())
            {
                Logger::LogError("Rigid bodies require a transform root");
                return Framework::LifecycleResult::kFailed;
            }
        }

        const auto* modifier = context.m_entity.FindComponent<const NonUniformScaleComponent>();
        const Vector3 scale = (modifier ? modifier->m_scale : Vector3(1)) * transform->m_local.Scale();
        if (!physics->CreateBody(*this, transform->m_local, scale, m_body))
        {
            Logger::LogError("Invalid rigid body definition");
            return Framework::LifecycleResult::kFailed;
        }

        m_lastTransform = transform->m_local;
        return Framework::LifecycleResult::kSucceeded;
    }


    void PhysicsBodyComponent::Deactivate(Framework::ComponentContext& context)
    {
        auto* physics = context.m_world.FindService<WorldPhysicsService>();
        physics->DestroyBody(m_body);
        m_body = UINT32_MAX;
    }
} // namespace FE::GameFramework
