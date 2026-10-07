#pragma once
#include <Framework/Entities/Query.h>
#include <GameFramework/TransformComponents.h>

namespace FE::GameFramework
{
    namespace Phases
    {
        FE_DECLARE_ENTITY_PHASE(Transformation);
    } // namespace Phases


    struct TransformationSystem final : Framework::WorldSystem
    {
        void Init(Framework::EntityWorld& world) override;
        void Update(Framework::EntityUpdateContext& context) override;
        [[nodiscard]] const Rc<WaitGroup>& GetCompletion() const
        {
            return m_completion;
        }

    private:
        Framework::ChangeCursor m_changes;
        Rc<WaitGroup> m_completion;
    };
} // namespace FE::GameFramework
