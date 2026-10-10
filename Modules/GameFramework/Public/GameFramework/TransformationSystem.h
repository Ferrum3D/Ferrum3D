#pragma once
#include <Framework/Entities/Query.h>
#include <GameFramework/Phases.h>
#include <GameFramework/TransformComponents.h>

namespace FE::GameFramework
{
    //! @brief Register transform layouts and publish parent-before-child world matrices through a cascade query.
    struct TransformationSystem final : Framework::WorldSystem
    {
        FE_RTTI("49C7EDBB-D164-4C26-BA2A-AB53F1397406");

        void Init(Framework::EntityWorld& world) override;
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
