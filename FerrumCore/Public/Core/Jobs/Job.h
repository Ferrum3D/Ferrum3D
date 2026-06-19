#pragma once
#include <Core/Containers/ConcurrentQueue.h>
#include <Core/Jobs/WaitGroup.h>
#include <Core/Memory/RefCount.h>

namespace FE
{
    struct Job : public ConcurrentQueue::Node
    {
        Job() = default;
        virtual ~Job();

        Job(const Job&) = delete;
        Job& operator=(const Job&) = delete;
        Job(Job&&) = delete;
        Job& operator=(Job&&) = delete;

        virtual void Execute() = 0;

        void AddPrerequisite(WaitGroup* waitGroup);
        void AddPrerequisite(const Rc<WaitGroup>& waitGroup)
        {
            AddPrerequisite(waitGroup.Get());
        }

        void AddPrerequisites(festd::span<WaitGroup* const> waitGroups);
        void AddPrerequisites(festd::span<const Rc<WaitGroup>> waitGroups);

        void AddPrerequisites(const std::initializer_list<WaitGroup*> waitGroups)
        {
            AddPrerequisites(festd::span(waitGroups));
        }

        void AddPrerequisites(const std::initializer_list<const Rc<WaitGroup>> waitGroups)
        {
            AddPrerequisites(festd::span(waitGroups));
        }

        void Schedule(IJobSystem* jobSystem, FiberAffinityMask affinityMask, WaitGroup* completionWaitGroup = nullptr,
                      JobPriority priority = JobPriority::kNormal);

        void ScheduleForeground(IJobSystem* jobSystem, WaitGroup* completionWaitGroup = nullptr,
                                const JobPriority priority = JobPriority::kNormal)
        {
            Schedule(jobSystem, FiberAffinityMask::kAll, completionWaitGroup, priority);
        }

        void ScheduleBackground(IJobSystem* jobSystem, WaitGroup* completionWaitGroup = nullptr,
                                const JobPriority priority = JobPriority::kNormal)
        {
            Schedule(jobSystem, FiberAffinityMask::kAllBackground, completionWaitGroup, priority);
        }

    private:
        friend struct JobSystem;
        friend struct WaitGroup;

        bool DependencySatisfied()
        {
            const uint32_t previousValue = m_dependencyCounter.fetch_sub(1, std::memory_order_acq_rel);
            FE_Assert(previousValue > 0, "Job dependency counter underflow");
            return previousValue == 1;
        }

        Rc<WaitGroup> m_completionWaitGroup;
        IJobSystem* m_jobSystem = nullptr;
        std::atomic<uint32_t> m_dependencyCounter = 1;
        std::atomic<bool> m_scheduleRequested = false;
        JobPriority m_priority = JobPriority::kNormal;
        FiberAffinityMask m_affinityMask = FiberAffinityMask::kNone;
        uint64_t m_orderHint = 0;
    };


    template<class TFunc>
    struct FunctorJob final : public Job
    {
        explicit FunctorJob(TFunc&& func)
            : m_func(std::move(func))
        {
        }

        void Execute() override
        {
            m_func();
        }

        TFunc m_func;
    };
} // namespace FE
