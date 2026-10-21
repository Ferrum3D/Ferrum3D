#include <Framework/Entities/Query.h>
#include <GameFramework/Components/CameraComponent.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Systems/CameraSystem.h>

namespace FE::GameFramework
{
    void CameraSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<CameraComponent>();
        m_changes = {};
    }


    void CameraSystem::Update(Framework::EntityUpdateContext& context)
    {
        FE_PROFILER_ZONE();

        const auto traverse = [](const CameraComponent& camera, const WorldTransformComponent& transform) {
            Vector3 position, scale, shear;
            Quaternion rotation;
            if (!Math::DecomposeTransform(transform.m_world, position, rotation, scale, shear))
                return;

            camera.m_view->SetCameraTransform(Transform::Create(position, rotation, 1.0f));
            camera.m_view->SetProjection(camera.m_fovY, camera.m_aspectRatio, camera.m_nearPlane, camera.m_farPlane);
        };

        using Query = Framework::Query<const CameraComponent, const WorldTransformComponent>;
        Query::Create()
            .SetChangeCursor(m_changes)
            .SetExecutionPolicy(Framework::ExecutionPolicy::kMainThread)
            .Traverse(context, Phases::GraphicsExtraction, traverse);
    }
} // namespace FE::GameFramework
