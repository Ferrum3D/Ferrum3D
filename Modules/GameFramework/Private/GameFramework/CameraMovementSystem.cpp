#include <Framework/Entities/Query.h>
#include <GameFramework/CameraComponent.h>
#include <GameFramework/CameraMovementSystem.h>
#include <GameFramework/Phases.h>
#include <cmath>

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
        m_tables.clear();
        m_input = nullptr;
    }


    Input::MappingStack& CameraMovementSystem::GetStack(const IO::Link<Input::MappingTable>& table)
    {
        for (TableRuntime& entry : m_tables)
        {
            if (entry.m_id == table.GetAssetID())
            {
                entry.m_used = true;
                return entry.m_stack;
            }
        }

        m_tables.push_back({});
        auto& entry = m_tables.back();
        entry.m_id = table.GetAssetID();
        entry.m_used = true;
        entry.m_stack.PushTable(table.GetAssetHandle(), Input::MappingStackFlags::kEnabled);
        return entry.m_stack;
    }


    void CameraMovementSystem::Update(Framework::EntityUpdateContext& context)
    {
        for (auto it = m_tables.begin(); it != m_tables.end();)
        {
            if (!it->m_used)
                it = m_tables.erase(it);
            else
            {
                it->m_used = false;
                ++it;
            }
        }

        using Cameras = Framework::Query<const CameraMovementComponent, const CameraComponent, TransformComponent>;
        Cameras::Traverse(context,
                          Phases::Input,
                          Framework::ExecutionPolicy::kMainThread,
                          [this](const CameraMovementComponent& movement, const CameraComponent&, TransformComponent& transform) {
                              auto& stack = GetStack(movement.m_mappingTable);
                              const auto actions = m_input->Resolve(stack);
                              ApplyMovement(movement, transform, actions, m_input->GetDeltaSeconds());
                          });
    }


    void CameraMovementSystem::ApplyMovement(const CameraMovementComponent& movement, TransformComponent& transform,
                                             const festd::span<const Input::ResolvedAction> actions, const float deltaSeconds)
    {
        Vector2 planar{ 0, 0 }, look{ 0, 0 };
        float elevation = 0;
        for (const Input::ResolvedAction& action : actions)
        {
            if (!action.m_active)
                continue;

            if (action.m_action == movement.m_moveAction.GetAssetID() && action.m_type == Input::ActionType::kVector2)
                planar = action.m_value;
            else if (action.m_action == movement.m_elevateAction.GetAssetID() && action.m_type == Input::ActionType::kScalar)
                elevation = action.m_value.x;
            else if (action.m_action == movement.m_lookAction.GetAssetID() && action.m_type == Input::ActionType::kVector2)
                look = action.m_value;
        }

        if (!std::isfinite(deltaSeconds) || deltaSeconds < 0 || !std::isfinite(movement.m_moveSpeed)
            || !std::isfinite(movement.m_mouseSensitivity))
        {
            return;
        }

        Quaternion rotation = transform.m_local.Rotation();
        if (look.x != 0 || look.y != 0)
        {
            const Quaternion yaw = Quaternion::AxisAngle(Vector3{ 0, 1, 0 }, look.x * movement.m_mouseSensitivity);
            const Quaternion pitch = Quaternion::AxisAngle(Vector3{ 1, 0, 0 }, look.y * movement.m_mouseSensitivity);
            rotation = Math::Normalize(yaw * rotation * pitch);
        }

        const Vector3 localMotion{ planar.x, 0, planar.y };
        const Vector3 motion = Math::Rotate(localMotion, rotation) + Vector3{ 0, elevation, 0 };
        const Vector3 position = transform.m_local.Translation() + motion * (movement.m_moveSpeed * deltaSeconds);
        transform.m_local = Transform::Create(position, rotation, transform.m_local.Scale());
    }
} // namespace FE::GameFramework
