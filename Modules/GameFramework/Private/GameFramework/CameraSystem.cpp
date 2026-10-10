#include <Framework/Entities/Query.h>
#include <GameFramework/CameraComponent.h>
#include <GameFramework/CameraSystem.h>
#include <GameFramework/TransformComponents.h>

namespace FE::GameFramework
{
    void CameraSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<CameraComponent>();
        m_changes = {};
    }


    void CameraSystem::Update(Framework::EntityUpdateContext& context)
    {
        using Cameras = Framework::Query<const CameraComponent, const WorldTransformComponent>;
        Cameras::TraverseChanged(
            context,
            Phases::GraphicsExtraction,
            m_changes,
            [](const CameraComponent& camera, const WorldTransformComponent& transform) {
                if (!camera.Validate())
                    return;

                Vector3 position, scale, shear;
                Quaternion rotation;
                if (!Math::DecomposeTransform(transform.m_world, position, rotation, scale, shear))
                    return;

                camera.m_view->SetCameraTransform(Transform::Create(position, rotation, 1.0f));
                camera.m_view->SetProjection(camera.m_fovY, camera.m_aspectRatio, camera.m_nearPlane, camera.m_farPlane);
            },
            Framework::ExecutionPolicy::kMainThread);
    }
} // namespace FE::GameFramework
