#include <Framework/Entities/Query.h>
#include <GameFramework/Components/CameraComponent.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Editor/CameraMovementSystem.h>
#include <GameFramework/Phases.h>

namespace FE::Editor
{
    void CameraMovementSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<CameraMovementComponent>({ true });
    }


    void CameraMovementSystem::Update(Framework::EntityUpdateContext& context)
    {
        auto* input = context.m_world.FindService<Input::WorldInputService>();
        FE_Assert(input, "Editor camera movement requires WorldInputService");
        const bool enabled = m_inputEnabled;
        const float aspect = static_cast<float>(m_viewportSize.x) / Math::Max(1.0f, static_cast<float>(m_viewportSize.y));
        using Query =
            Framework::Query<const CameraMovementComponent, GameFramework::CameraComponent, GameFramework::TransformComponent>;
        Query::Create()
            .SetExecutionPolicy(Framework::ExecutionPolicy::kMainThread)
            .Traverse(context,
                      GameFramework::Phases::Input,
                      [input, enabled, aspect](const CameraMovementComponent& movement,
                                               GameFramework::CameraComponent& projection,
                                               GameFramework::TransformComponent& transform) {
                          bool changed = projection.m_aspectRatio != aspect;
                          projection.m_aspectRatio = aspect;
                          if (enabled)
                          {
                              const auto& frame = input->GetFrame();
                              const GameFramework::CameraInput values{ .m_move = frame.Read(movement.m_movement.m_moveAction),
                                                                       .m_elevate =
                                                                           frame.Read(movement.m_movement.m_elevateAction),
                                                                       .m_look = frame.Read(movement.m_movement.m_lookAction) };
                              const float speed = frame.Read(movement.m_speedUpAction) ? movement.m_speedUpFactor : 1;
                              changed |= GameFramework::ApplyCameraMovement(movement.m_movement,
                                                                            transform,
                                                                            values,
                                                                            input->GetDeltaSeconds(),
                                                                            speed)
                                  == Framework::QueryWriteResult::kChanged;
                          }

                          return changed ? Framework::QueryWriteResult::kChanged : Framework::QueryWriteResult::kUnchanged;
                      });
    }
} // namespace FE::Editor
