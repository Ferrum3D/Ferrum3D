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
        m_modelChanges = {};
        m_transformChanges = {};
    }


    void MeshSystem::Update(Framework::EntityUpdateContext& context)
    {
        FE_PROFILER_ZONE();

        auto* sceneService = context.m_world.FindService<WorldGraphicsSceneService>();

        const auto updateModel = [sceneService](MeshComponent& mesh) {
            sceneService->GetMeshes().UpdateModel(mesh);
            return Framework::QueryWriteResult::kUnchanged;
        };

        const auto models = Framework::Query<MeshComponent>::TraverseChanged(context,
                                                                             Phases::GraphicsExtraction,
                                                                             m_modelChanges,
                                                                             updateModel,
                                                                             Framework::ExecutionPolicy::kMainThread);
        const auto updateTransform = [sceneService](const MeshComponent& mesh, const WorldTransformComponent& transform) {
            sceneService->GetMeshes().UpdateTransform(mesh, transform.m_world);
        };

        using Query = Framework::Query<const MeshComponent, const WorldTransformComponent>;
        Query::Traverse(context,
                        Phases::GraphicsExtraction,
                        { models },
                        updateTransform,
                        Framework::ExecutionPolicy::kParallel,
                        &m_transformChanges);
    }
} // namespace FE::GameFramework
