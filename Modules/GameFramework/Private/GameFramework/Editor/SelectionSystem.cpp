#include <Framework/Entities/Query.h>
#include <GameFramework/Components/MeshComponent.h>
#include <GameFramework/Editor/SelectionSystem.h>
#include <GameFramework/Services/WorldGraphicsSceneService.h>
#include <Graphics/Features/Mesh/MeshSceneModule.h>

namespace FE::Editor
{
    void SelectionSystem::Update(Framework::EntityUpdateContext& context)
    {
        const auto* graphics = context.m_world.FindService<GameFramework::WorldGraphicsSceneService>();
        auto* selection = m_view ? m_view->GetModules().TryFind<SelectionViewModule>() : nullptr;
        if (!graphics || !selection)
            return;

        const auto* meshes = graphics->GetScene().GetModules().TryFind<Graphics::MeshSceneModule>();
        if (!meshes)
            return;

        const bool retainBindings = selection->IsPickPending();
        if (!retainBindings)
            m_bindings.clear();

        m_selectedObjects.clear();
        Framework::Query<const GameFramework::MeshComponent>::Create()
            .SetExecutionPolicy(Framework::ExecutionPolicy::kMainThread)
            .Traverse(context,
                      Phases::Selection,
                      [this, meshes, retainBindings](const Framework::Entity& entity, const GameFramework::MeshComponent& mesh) {
                          for (const auto handle : mesh.m_handles)
                          {
                              const uint32_t object = meshes->GetVisibilityID(handle);
                              if (!object)
                                  continue;

                              if (!retainBindings)
                                  m_bindings.push_back({ object, entity.GetID() });

                              if (entity.GetID() == m_selected)
                                  m_selectedObjects.push_back(object);
                          }
                      });
        context.m_world.RecordStage(Phases::Selection, [this, selection] {
            selection->SetSelection(m_selectedObjects);
        });
    }


    Framework::EntityID SelectionSystem::FindEntity(const uint32_t object) const
    {
        for (const auto& binding : m_bindings)
        {
            if (binding.m_object == object)
                return binding.m_entity;
        }

        return {};
    }


    void SelectionSystem::Reset()
    {
        m_bindings.clear();
        m_selectedObjects.clear();
        m_selected = {};
        m_view = nullptr;
    }
} // namespace FE::Editor
