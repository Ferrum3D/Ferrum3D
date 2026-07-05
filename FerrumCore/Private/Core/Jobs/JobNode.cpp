#include <Core/Jobs/JobNode.h>
#include <Core/Jobs/JobSystem.h>

namespace FE::Jobs
{
    JobNode::~JobNode() = default;


    void JobNode::AddPrerequisite(WaitGroup* waitGroup)
    {
        FE_AssertDebug(waitGroup != nullptr, "Job prerequisite cannot be null");
        FE_AssertDebug(!m_dispatchRequested.load(std::memory_order_acquire), "Prerequisites must be added before dispatching");

        const uint32_t previousValue = m_dependencyCounter.fetch_add(1, std::memory_order_relaxed);
        FE_AssertDebug(previousValue < Constants::kMaxU32, "Too many job prerequisites");
        waitGroup->AddJobPrerequisite(this);
    }


    void JobNode::AddPrerequisites(const festd::span<WaitGroup* const> waitGroups)
    {
        for (WaitGroup* waitGroup : waitGroups)
            AddPrerequisite(waitGroup);
    }


    void JobNode::AddPrerequisites(const festd::span<const Rc<WaitGroup>> waitGroups)
    {
        for (const Rc<WaitGroup>& waitGroup : waitGroups)
            AddPrerequisite(waitGroup);
    }


    void JobNode::Dispatch(const FiberAffinityMask affinityMask, WaitGroup* completionWaitGroup, const Priority priority)
    {
        if (completionWaitGroup)
            m_completionWaitGroup = completionWaitGroup;

        m_priority = priority;
        m_affinityMask = affinityMask;
        JobSystem::Get().Dispatch(this);
    }
} // namespace FE::Jobs
