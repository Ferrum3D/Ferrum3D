#include <Framework/Input/Action.h>
#include <cmath>

namespace FE::Input
{
    ActionState HoldDurationActionProcessor::Process(ActionProcessorContext& context) const
    {
        if (!context.m_active)
            return context.m_completed ? ActionState::kCompleted : ActionState::kCanceled;

        context.m_heldSeconds += context.m_deltaSeconds;
        if (context.m_heldSeconds >= std::fmax(0.0f, m_holdTime))
        {
            context.m_completed = true;
            return ActionState::kTriggered;
        }

        return context.m_wasActive ? ActionState::kInProgress : ActionState::kStarted;
    }


    ActionState ContinuousActionProcessor::Process(ActionProcessorContext& context) const
    {
        if (!context.m_active)
            return ActionState::kCompleted;

        return ActionState::kTriggered;
    }


    bool Action::Validate() const
    {
        if (m_type > ActionType::kVector2)
            return false;

        const auto* hold = Rtti::Cast<const HoldDurationActionProcessor*>(m_processor.Get());
        return hold == nullptr || (std::isfinite(hold->m_holdTime) && hold->m_holdTime >= 0);
    }
} // namespace FE::Input
