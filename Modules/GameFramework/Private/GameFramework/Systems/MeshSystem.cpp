#include <Framework/Entities/Query.h>
#include <GameFramework/Components/MeshComponent.h>
#include <GameFramework/Components/PhysicsBodyComponent.h>
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

        const auto updateModel =
            [sceneService](MeshComponent& mesh, const DynamicTagComponent* tag, const PhysicsBodyComponent* body) {
                MeshSceneBridge& meshes = sceneService->GetMeshes();
                meshes.UpdateModel(mesh);

                const Graphics::MeshMobility mobility = GetMeshMobility(tag, body);
                if (mobility != mesh.m_mobility)
                    meshes.UpdateMobility(mesh, mobility);

                return Framework::QueryWriteResult::kUnchanged;
            };

        using ModelQuery = Framework::Query<MeshComponent, const DynamicTagComponent*, const PhysicsBodyComponent*>;
        const auto models = ModelQuery::Create()
                                .SetChangeCursor(m_modelChanges)
                                .SetExecutionPolicy(Framework::ExecutionPolicy::kMainThread)
                                .Traverse(context, Phases::GraphicsExtraction, updateModel);
        const auto updateTransform = [sceneService](const MeshComponent& mesh, const WorldTransformComponent& transform) {
            sceneService->GetMeshes().UpdateTransform(mesh, transform.m_world);
        };

        using Query = Framework::Query<const MeshComponent, const WorldTransformComponent>;
        Query::Create()
            .SetPrerequisites({ models })
            .SetExecutionPolicy(Framework::ExecutionPolicy::kParallel)
            .SetChangeCursor(m_transformChanges)
            .Traverse(context, Phases::GraphicsExtraction, updateTransform);
    }
} // namespace FE::GameFramework
