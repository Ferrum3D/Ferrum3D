#include <Framework/Entities/Query.h>
#include <GameFramework/CameraComponent.h>
#include <GameFramework/CameraMovementSystem.h>
#include <GameFramework/Phases.h>

namespace FE::GameFramework
{
    void CameraMovementSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<CameraMovementComponent>();
        m_input = world.FindService<Input::WorldInputService>();

        FE_Assert(m_input != nullptr, "Camera movement requires WorldInputService");
    }


    void CameraMovementSystem::Shutdown(Framework::EntityWorld&)
    {
        m_input = nullptr;
    }


    void CameraMovementSystem::Update(Framework::EntityUpdateContext& context)
    {
        FE_PROFILER_ZONE();

        const auto traverse =
            [this](const CameraMovementComponent& movement, const CameraComponent&, TransformComponent& transform) {
                const Input::InputFrame& frame = m_input->GetFrame();

                const CameraInput values{ .m_move = frame.Read(movement.m_moveAction),
                                          .m_elevate = frame.Read(movement.m_elevateAction),
                                          .m_look = frame.Read(movement.m_lookAction) };

                return ApplyMovement(movement, transform, values, m_input->GetDeltaSeconds())
                    ? Framework::QueryWriteResult::kChanged
                    : Framework::QueryWriteResult::kUnchanged;
            };

        using Query = Framework::Query<const CameraMovementComponent, const CameraComponent, TransformComponent>;
        Query::Traverse(context, Phases::Input, Framework::ExecutionPolicy::kMainThread, traverse);
    }


    bool CameraMovementSystem::ApplyMovement(const CameraMovementComponent& movement, TransformComponent& transform,
                                             const CameraInput& input, const float deltaSeconds)
    {
        FE_Assert(deltaSeconds >= 0);

        const bool translate =
            deltaSeconds > 0 && movement.m_moveSpeed != 0 && (input.m_move != Vector2::kZero || input.m_elevate != 0);
        const bool rotate = input.m_look != Vector2::kZero;
        if (!translate && !rotate)
            return false;

        Quaternion rotation = transform.m_local.Rotation();
        if (rotate)
        {
            const Quaternion yaw = Quaternion::RotationY(input.m_look.x);
            const Quaternion pitch = Quaternion::RotationX(input.m_look.y);
            rotation = Math::Normalize(yaw * rotation * pitch);
        }

        const Vector3 localMotion{ input.m_move.x, 0, input.m_move.y };
        const Vector3 motion = Math::Rotate(localMotion, rotation) + Vector3{ 0, input.m_elevate, 0 };
        const Vector3 position = transform.m_local.Translation() + motion * (movement.m_moveSpeed * deltaSeconds);
        transform.m_local = Transform::Create(position, rotation, transform.m_local.Scale());
        return true;
    }
} // namespace FE::GameFramework
