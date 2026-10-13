#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/Components/MeshComponent.h>
#include <GameFramework/Services/WorldGraphicsSceneService.h>

namespace FE::GameFramework
{
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

        return sceneService->GetMeshes().Create(*this);
    }


    void MeshComponent::Deactivate(Framework::ComponentContext& context)
    {
        auto* sceneService = context.m_world.FindService<WorldGraphicsSceneService>();
        FE_Assert(sceneService);

        sceneService->GetMeshes().Destroy(*this);
    }
} // namespace FE::GameFramework
