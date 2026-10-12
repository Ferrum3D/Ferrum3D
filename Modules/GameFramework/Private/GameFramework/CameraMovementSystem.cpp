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
        m_bindings.clear();
        m_input = nullptr;
    }


    CameraInput CameraMovementSystem::ReadInput(const Framework::Entity& entity, const CameraMovementComponent& movement)
    {
        auto [it, inserted] = m_bindings.try_emplace(entity.GetUuid());
        
        Bindings& bindings = it->second;
        if (inserted || bindings.m_moveID != movement.m_moveAction.GetAssetID()
            || bindings.m_elevateID != movement.m_elevateAction.GetAssetID()
            || bindings.m_lookID != movement.m_lookAction.GetAssetID())
        {
            bindings.m_moveID = movement.m_moveAction.GetAssetID();
            bindings.m_elevateID = movement.m_elevateAction.GetAssetID();
            bindings.m_lookID = movement.m_lookAction.GetAssetID();
            bindings.m_move = m_input->Bind<Vector2>(movement.m_moveAction);
            bindings.m_elevate = m_input->Bind<float>(movement.m_elevateAction);
            bindings.m_look = m_input->Bind<Vector2>(movement.m_lookAction);
        }

        const Input::InputFrame& frame = m_input->GetFrame();
        bindings.m_frame = frame.GetNumber();
        return { frame.Read(bindings.m_move), frame.Read(bindings.m_elevate), frame.Read(bindings.m_look) };
    }


    void CameraMovementSystem::Update(Framework::EntityUpdateContext& context)
    {
        const uint64_t number = m_input->GetFrame().GetNumber();
        for (auto it = m_bindings.begin(); it != m_bindings.end();)
        {
            if (it->second.m_frame + 1 < number)
                it = m_bindings.erase(it);
            else
                ++it;
        }

        using Cameras = Framework::Query<const CameraMovementComponent, const CameraComponent, TransformComponent>;
        Cameras::Traverse(context,
                          Phases::Input,
                          Framework::ExecutionPolicy::kMainThread,
                          [this](Framework::Entity& entity,
                                 const CameraMovementComponent& movement,
                                 const CameraComponent&,
                                 TransformComponent& transform) {
                              return ApplyMovement(movement, transform, ReadInput(entity, movement), m_input->GetDeltaSeconds());
                          });
    }


    bool CameraMovementSystem::ApplyMovement(const CameraMovementComponent& movement, TransformComponent& transform,
                                             const CameraInput& input, const float deltaSeconds)
    {
        if (!std::isfinite(deltaSeconds) || deltaSeconds < 0 || !std::isfinite(movement.m_moveSpeed))
            return false;

        if (!std::isfinite(input.m_move.x) || !std::isfinite(input.m_move.y) || !std::isfinite(input.m_elevate)
            || !std::isfinite(input.m_look.x) || !std::isfinite(input.m_look.y))
        {
            return false;
        }

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
