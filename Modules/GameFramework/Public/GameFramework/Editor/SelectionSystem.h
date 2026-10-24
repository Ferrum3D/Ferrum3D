#pragma once
#include <Framework/Entities/EntityWorld.h>
#include <GameFramework/Editor/Selection.h>

namespace FE::Editor
{
    namespace Phases
    {
        FE_DECLARE_ENTITY_PHASE(Selection);
    }


    struct SelectionSystem final : Framework::WorldSystem
    {
        FE_RTTI("439E825E-5049-4D4F-86A3-F943460F185F");

        void Update(Framework::EntityUpdateContext& context) override;
        Framework::EntityID FindEntity(uint32_t object) const;
        void Reset();

        Framework::EntityID m_selected;
        Graphics::View* m_view = nullptr;

    private:
        struct Binding final
        {
            uint32_t m_object;
            Framework::EntityID m_entity;
        };

        festd::vector<Binding> m_bindings;
        festd::inline_vector<uint32_t, 8> m_selectedObjects;
    };
} // namespace FE::Editor
