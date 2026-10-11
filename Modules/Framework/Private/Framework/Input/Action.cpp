#include <Framework/Input/Action.h>
#include <cmath>

namespace FE::Input
{
    Serialization::ResultCode ActionProcessor::Serialize(Serialization::SerializationContext& context) const
    {
        // Reference counts are runtime ownership, not authored processor state.
        if (auto object = context.BeginObject())
        {
        }

        return context.GetResultCode();
    }


    Serialization::ResultCode ActionProcessor::Deserialize(Serialization::DeserializationContext& context)
    {
        if (auto object = context.BeginObject())
        {
        }

        return context.GetResultCode();
    }


    void ActionProcessor::DoRelease()
    {
        Memory::DefaultDelete(this);
    }


    ActionState HoldDurationActionProcessor::Process(ActionProcessorContext& context) const
    {
        if (!context.m_active)
            return context.m_completed ? ActionState::kFinished : ActionState::kCanceled;

        context.m_heldSeconds += context.m_deltaSeconds;
        if (context.m_heldSeconds >= std::fmax(0.0f, m_holdTime))
        {
            context.m_completed = true;
            return ActionState::kFinished;
        }

        return context.m_wasActive ? ActionState::kInProgress : ActionState::kStarted;
    }


    ActionState ContinuousActionProcessor::Process(ActionProcessorContext& context) const
    {
        if (!context.m_active)
            return ActionState::kFinished;

        return context.m_wasActive ? ActionState::kInProgress : ActionState::kStarted;
    }
} // namespace FE::Input
