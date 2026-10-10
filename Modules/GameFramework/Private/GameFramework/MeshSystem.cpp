#include <Framework/Entities/Query.h>
#include <GameFramework/MeshComponent.h>
#include <GameFramework/MeshSystem.h>
#include <GameFramework/TransformComponents.h>
#include <GameFramework/WorldGraphicsSceneService.h>

namespace FE::GameFramework
{
    void MeshSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<MeshComponent>();
        m_changes = {};
    }


    void MeshSystem::Update(Framework::EntityUpdateContext& context)
    {
        using Meshes = Framework::Query<MeshComponent, const WorldTransformComponent>;
        Meshes::TraverseChanged(
            context,
            Phases::GraphicsExtraction,
            m_changes,
            [](MeshComponent& mesh, const WorldTransformComponent& transform) {
                mesh.m_scene->GetMeshes().Update(mesh, transform.m_world);
            },
            Framework::ExecutionPolicy::kMainThread);
    }
} // namespace FE::GameFramework
