#include <Framework/Entities/Query.h>
#include <Framework/Entities/WorldTimeService.h>
#include <GameFramework/Components/CameraComponent.h>
#include <GameFramework/Components/CameraMovementComponent.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Phases.h>
#include <GameFramework/Systems/CameraMovementSystem.h>

namespace FE::GameFramework
{
    Framework::QueryWriteResult ApplyCameraMovement(const CameraMovementComponent& movement, TransformComponent& transform,
                                                    const CameraInput& input, const float deltaSeconds, const float speedScale)
    {
        FE_Assert(deltaSeconds >= 0);

        const bool translate =
            deltaSeconds > 0 && movement.m_moveSpeed != 0 && (input.m_move != Vector2::kZero || input.m_elevate != 0);
        const bool rotate = input.m_look != Vector2::kZero;
        if (!translate && !rotate)
            return Framework::QueryWriteResult::kUnchanged;

        Quaternion rotation = transform.m_local.Rotation();
        if (rotate)
        {
            const Quaternion yaw = Quaternion::RotationY(input.m_look.x);
            const Quaternion pitch = Quaternion::RotationX(input.m_look.y);
            rotation = Math::Normalize(yaw * rotation * pitch);
        }

        const Vector3 localMotion{ input.m_move.x, 0, input.m_move.y };
        const Vector3 motion = Math::Rotate(localMotion, rotation) + Vector3{ 0, input.m_elevate, 0 };
        const Vector3 position = transform.m_local.Translation() + motion * (movement.m_moveSpeed * deltaSeconds * speedScale);
        transform.m_local = Transform::Create(position, rotation, transform.m_local.Scale());
        return Framework::QueryWriteResult::kChanged;
    }


    void CameraMovementSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<CameraMovementComponent>();
    }


    void CameraMovementSystem::Update(Framework::EntityUpdateContext& context)
    {
        FE_PROFILER_ZONE();

        const auto* time = context.m_world.FindService<Framework::WorldTimeService>();
        if (time && time->IsPaused())
            return;

        auto* inputService = context.m_world.FindService<Input::WorldInputService>();
        FE_Assert(inputService != nullptr, "Camera movement requires WorldInputService");

        const auto traverse =
            [inputService](const CameraMovementComponent& movement, const CameraComponent&, TransformComponent& transform) {
                const Input::InputFrame& frame = inputService->GetFrame();

                const CameraInput values{ .m_move = frame.Read(movement.m_moveAction),
                                          .m_elevate = frame.Read(movement.m_elevateAction),
                                          .m_look = frame.Read(movement.m_lookAction) };

                return ApplyCameraMovement(movement, transform, values, inputService->GetDeltaSeconds());
            };

        using Query = Framework::Query<const CameraMovementComponent, const CameraComponent, TransformComponent>;
        Query::Create().SetExecutionPolicy(Framework::ExecutionPolicy::kMainThread).Traverse(context, Phases::Input, traverse);
    }
} // namespace FE::GameFramework
