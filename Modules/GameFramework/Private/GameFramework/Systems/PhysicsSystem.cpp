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

        using InputsQuery = Query<const PhysicsBodyComponent, const TransformComponent>;
        InputsQuery::Create()
            .SetExecutionPolicy(ExecutionPolicy::kMainThread)
            .Traverse(context,
                      Framework::Phases::PreUpdate,
                      [physics, deltaSeconds](const PhysicsBodyComponent& body, const TransformComponent& transform) {
                          const bool changed = transform.m_local == body.m_lastTransform;
                          if (changed || body.m_motionType == PhysicsMotionType::kKinematic)
                              physics->SetTransform(body.m_body, transform.m_local, body.m_motionType, deltaSeconds);
                      });

        context.m_world.RecordStage(Framework::Phases::Update, [physics, deltaSeconds] {
            physics->Step(deltaSeconds);
        });

        using OutputsQuery = Query<PhysicsBodyComponent, TransformComponent>;
        OutputsQuery::Create()
            .SetExecutionPolicy(ExecutionPolicy::kMainThread)
            .Traverse(context,
                      Framework::Phases::PostUpdate,
                      [physics](PhysicsBodyComponent& body, TransformComponent& transform) {
                          if (body.m_motionType == PhysicsMotionType::kDynamic)
                          {
                              transform.m_local = physics->GetTransform(body.m_body, transform.m_local.Scale());
                              physics->GetVelocity(body.m_body, body.m_linearVelocity, body.m_angularVelocity);
                          }

                          const bool changed = transform.m_local == body.m_lastTransform;
                          body.m_lastTransform = transform.m_local;
                          return changed ? QueryWriteResult::kChanged : QueryWriteResult::kUnchanged;
                      });
    }
} // namespace FE::GameFramework
