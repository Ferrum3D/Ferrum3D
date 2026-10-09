#include <GameFramework/GraphicsSystems.h>

namespace FE::GameFramework
{
    void CameraSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<CameraComponent>();
        const Rtti::TypeID dependencies[] = { Rtti::GetTypeID<CameraComponent>() };
        world.Components().Register<CameraRuntimeComponent>(dependencies, { true });
        world.Components().AddRuntimeCompanion<CameraComponent, CameraRuntimeComponent>();
        m_changes = {};
    }


    void CameraSystem::Update(Framework::EntityUpdateContext& context)
    {
        m_pending.clear();
        using Cameras = Framework::Query<const CameraComponent, const WorldTransformComponent, const CameraRuntimeComponent>;
        auto collected = Cameras::TraverseChanged(context,
                                                  Phases::GraphicsExtraction,
                                                  m_changes,
                                                  [this](Framework::Entity& entity,
                                                         const CameraComponent&,
                                                         const WorldTransformComponent&,
                                                         const CameraRuntimeComponent&) {
                                                      m_pending.push_back(entity.GetID());
                                                  });
        const Rc<WaitGroup> prerequisites[] = { collected };
        context.m_world.RecordStage(
            Phases::GraphicsExtraction,
            [this, &world = context.m_world] {
                FE_PROFILER_ZONE();
                for (const auto id : m_pending)
                {
                    Framework::Entity& entity = *world.Find(id);
                    const auto& camera = *entity.FindComponent<const CameraComponent>();
                    const auto& transform = *entity.FindComponent<const WorldTransformComponent>();
                    auto& runtime = *entity.FindComponent<CameraRuntimeComponent>();
                    if (!camera.Validate())
                        continue;

                    Vector3 position, scale, shear;
                    Quaternion rotation;
                    if (!Math::DecomposeTransform(transform.m_world, position, rotation, scale, shear))
                        continue;

                    runtime.m_view->SetCameraTransform(Transform::Create(position, rotation, 1.0f));
                    runtime.m_view->SetProjection(camera.m_fovY, camera.m_aspectRatio, camera.m_nearPlane, camera.m_farPlane);
                }
            },
            prerequisites);
    }


    void MeshSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<MeshComponent>();
        const Rtti::TypeID dependencies[] = { Rtti::GetTypeID<MeshComponent>() };
        world.Components().Register<MeshRuntimeComponent>(dependencies, { true });
        world.Components().AddRuntimeCompanion<MeshComponent, MeshRuntimeComponent>();
        m_changes = {};
    }


    void MeshSystem::Update(Framework::EntityUpdateContext& context)
    {
        m_pending.clear();
        using Meshes = Framework::Query<const MeshComponent, const WorldTransformComponent, const MeshRuntimeComponent>;
        auto collected = Meshes::TraverseChanged(
            context,
            Phases::GraphicsExtraction,
            m_changes,
            [this](Framework::Entity& entity, const MeshComponent&, const WorldTransformComponent&, const MeshRuntimeComponent&) {
                m_pending.push_back(entity.GetID());
            });
        const Rc<WaitGroup> prerequisites[] = { collected };
        context.m_world.RecordStage(
            Phases::GraphicsExtraction,
            [this, &world = context.m_world] {
                FE_PROFILER_ZONE();
                for (const auto id : m_pending)
                {
                    Framework::Entity& entity = *world.Find(id);
                    const auto& mesh = *entity.FindComponent<const MeshComponent>();
                    const auto& transform = *entity.FindComponent<const WorldTransformComponent>();
                    auto& runtime = *entity.FindComponent<MeshRuntimeComponent>();
                    runtime.m_scene->GetMeshes().Update(mesh, transform.m_world, runtime);
                }
            },
            prerequisites);
    }
} // namespace FE::GameFramework
