#include <Framework/Entities/Query.h>
#include <GameFramework/Components/MeshComponent.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Services/WorldGraphicsSceneService.h>
#include <GameFramework/Systems/MeshSystem.h>

namespace FE::GameFramework
{
    void MeshSystem::Init(Framework::EntityWorld& world)
    {
        world.Components().Register<MeshComponent>();
        m_changes = {};
    }


    void MeshSystem::Update(Framework::EntityUpdateContext& context)
    {
        FE_PROFILER_ZONE();

        auto* sceneService = context.m_world.FindService<WorldGraphicsSceneService>();

        const auto traverse = [sceneService](MeshComponent& mesh, const WorldTransformComponent& transform) {
            sceneService->GetMeshes().Update(mesh, transform.m_world);
            return Framework::QueryWriteResult::kUnchanged;
        };

        using Query = Framework::Query<MeshComponent, const WorldTransformComponent>;
        Query::TraverseChanged(context, Phases::GraphicsExtraction, m_changes, traverse, Framework::ExecutionPolicy::kMainThread);
    }
} // namespace FE::GameFramework
