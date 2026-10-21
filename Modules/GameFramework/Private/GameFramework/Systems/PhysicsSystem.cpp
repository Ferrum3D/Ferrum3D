#include <Framework/Entities/Query.h>
#include <GameFramework/Components/PhysicsBodyComponent.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Services/WorldPhysicsService.h>
#include <GameFramework/Systems/PhysicsSystem.h>

namespace FE::GameFramework
{
    void PhysicsSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<TransformComponent>();
        world.Components().Register<NonUniformScaleComponent>();
        world.Components().Register<PhysicsBodyComponent>();
    }


    void PhysicsSystem::FixedUpdate(Framework::EntityUpdateContext& context)
    {
        using namespace Framework;

        auto* physics = context.m_world.FindService<WorldPhysicsService>();
        FE_AssertDebug(physics, "PhysicsSystem requires WorldPhysicsService");
        const float deltaSeconds = context.m_deltaSeconds;

        // Push authored transform edits; kinematic bodies receive a target every step.
        const auto pushInputs = [physics, deltaSeconds](const PhysicsBodyComponent& body, const TransformComponent& transform) {
            const bool edited = transform.m_local != body.m_lastTransform;
            if (edited || body.m_motionType == PhysicsMotionType::kKinematic)
                physics->SetTransform(body.m_body, transform.m_local, body.m_motionType, deltaSeconds);
        };

        // Publish simulated motion; only moving dynamic bodies report a transform change.
        const auto pullOutputs = [physics](PhysicsBodyComponent& body, TransformComponent& transform) {
            if (body.m_motionType == PhysicsMotionType::kDynamic)
            {
                transform.m_local = physics->GetTransform(body.m_body, transform.m_local.Scale());
                physics->GetVelocity(body.m_body, body.m_linearVelocity, body.m_angularVelocity);
            }

            const bool moved = body.m_motionType == PhysicsMotionType::kDynamic && transform.m_local != body.m_lastTransform;
            body.m_lastTransform = transform.m_local;
            return moved ? QueryWriteResult::kChanged : QueryWriteResult::kUnchanged;
        };

        Query<const PhysicsBodyComponent, const TransformComponent>::Create()
            .SetExecutionPolicy(ExecutionPolicy::kMainThread)
            .Traverse(context, Framework::Phases::PreUpdate, pushInputs);

        context.m_world.RecordStage(Framework::Phases::Update, [physics, deltaSeconds] {
            physics->Step(deltaSeconds);
        });

        Query<PhysicsBodyComponent, TransformComponent>::Create()
            .SetExecutionPolicy(ExecutionPolicy::kMainThread)
            .Traverse(context, Framework::Phases::PostUpdate, pullOutputs);
    }
} // namespace FE::GameFramework
