#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/Components/MeshComponent.h>
#include <GameFramework/Components/PhysicsBodyComponent.h>
#include <GameFramework/Components/TransformComponents.h>
#include <GameFramework/Services/WorldGraphicsSceneService.h>

namespace FE::GameFramework
{
    Graphics::MeshMobility GetMeshMobility(const DynamicTagComponent* tag, const PhysicsBodyComponent* body)
    {
        const bool isMovedByPhysics = body && body->m_motionType != PhysicsMotionType::kStatic;
        return tag || isMovedByPhysics ? Graphics::MeshMobility::kDynamic : Graphics::MeshMobility::kStatic;
    }


    MeshComponent::MeshComponent(IO::Link<Graphics::ModelAsset> model, IO::Link<Graphics::MaterialInstanceAsset> material)
        : m_model(model)
        , m_material(material)
    {
    }


    MeshComponent::MeshComponent(const MeshComponent& other)
        : m_model(other.m_model)
        , m_material(other.m_material)
    {
    }


    Framework::LifecycleResult MeshComponent::Activate(Framework::ComponentContext& context)
    {
        auto* sceneService = context.m_world.FindService<WorldGraphicsSceneService>();
        if (!sceneService)
            return Framework::LifecycleResult::kFailed;

        m_mobility = GetMeshMobility(context.m_entity.FindComponent<const DynamicTagComponent>(),
                                     context.m_entity.FindComponent<const PhysicsBodyComponent>());
        return sceneService->GetMeshes().Create(*this);
    }


    void MeshComponent::Deactivate(Framework::ComponentContext& context)
    {
        auto* sceneService = context.m_world.FindService<WorldGraphicsSceneService>();
        FE_Assert(sceneService);

        sceneService->GetMeshes().Destroy(*this);
    }
} // namespace FE::GameFramework
