#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/MeshComponent.h>
#include <GameFramework/WorldGraphicsSceneService.h>

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
        m_scene = context.m_world.FindService<WorldGraphicsSceneService>();
        if (!m_scene)
            return Framework::LifecycleResult::kFailed;

        return m_scene->GetMeshes().Create(*this);
    }


    void MeshComponent::Deactivate(Framework::ComponentContext&)
    {
        if (m_scene)
            m_scene->GetMeshes().Destroy(*this);

        m_scene = nullptr;
    }
} // namespace FE::GameFramework
