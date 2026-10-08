#pragma once
#include <Framework/Entities/Query.h>
#include <GameFramework/TransformComponents.h>

namespace FE::GameFramework
{
    namespace Phases
    {
        FE_DECLARE_ENTITY_PHASE(Transformation);
    } // namespace Phases


    //! @brief Register transform layouts and publish parent-before-child world matrices through a cascade query.
    struct TransformationSystem final : Framework::WorldSystem
    {
        //! @brief Register authored/runtime companions and install transactional transform reparenting.
        void Init(Framework::EntityWorld& world) override;
        //! @brief Detach the reparent handler before the system is removed.
        void Shutdown(Framework::EntityWorld& world) override;
        //! @brief Record changed transforms for this epoch; world matrices publish when its completion signals.
        void Update(Framework::EntityUpdateContext& context) override;
        //! @brief Borrow the latest transformation completion group for ordering later engine work.
        [[nodiscard]] const Rc<WaitGroup>& GetCompletion() const
        {
            return m_completion;
        }

    private:
        Framework::ChangeCursor m_changes;
        Rc<WaitGroup> m_completion;
    };
} // namespace FE::GameFramework
