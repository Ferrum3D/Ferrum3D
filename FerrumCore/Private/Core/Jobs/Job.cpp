#include <Core/Jobs/IJobSystem.h>
#include <Core/Jobs/Job.h>

namespace FE
{
    Job::~Job() = default;


    void Job::AddPrerequisite(WaitGroup* waitGroup)
    {
        FE_AssertDebug(waitGroup != nullptr, "Job prerequisite cannot be null");
        FE_AssertDebug(!m_dispatchRequested.load(std::memory_order_acquire), "Prerequisites must be added before dispatching");

        const uint32_t previousValue = m_dependencyCounter.fetch_add(1, std::memory_order_relaxed);
        FE_AssertDebug(previousValue < Constants::kMaxU32, "Too many job prerequisites");
        waitGroup->AddJobPrerequisite(this);
    }


    void Job::AddPrerequisites(const festd::span<WaitGroup* const> waitGroups)
    {
        for (WaitGroup* waitGroup : waitGroups)
            AddPrerequisite(waitGroup);
    }


    void Job::AddPrerequisites(const festd::span<const Rc<WaitGroup>> waitGroups)
    {
        for (const Rc<WaitGroup>& waitGroup : waitGroups)
            AddPrerequisite(waitGroup);
    }


    void Job::Dispatch(IJobSystem* jobSystem, const FiberAffinityMask affinityMask, WaitGroup* completionWaitGroup,
                       const JobPriority priority)
    {
        if (completionWaitGroup)
            m_completionWaitGroup = completionWaitGroup;

        JobDispatchInfo info;
        info.m_job = this;
        info.m_priority = priority;
        info.m_affinityMask = affinityMask;
        jobSystem->Dispatch(info);
    }
} // namespace FE
