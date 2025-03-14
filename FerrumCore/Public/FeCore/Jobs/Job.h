#pragma once
#include <FeCore/Containers/ConcurrentQueue.h>
#include <FeCore/Jobs/IJobSystem.h>
#include <FeCore/Jobs/WaitGroup.h>
#include <FeCore/Memory/Memory.h>
#include <festd/intrusive_list.h>

namespace FE
{
    struct Job
        : public festd::intrusive_list_node
        , public ConcurrentQueueNode
    {
        Job() = default;
        virtual ~Job() = default;

        Job(const Job&) = delete;
        Job& operator=(const Job&) = delete;
        Job(Job&&) = delete;
        Job& operator=(Job&&) = delete;

        virtual void Execute() = 0;

        void Schedule(IJobSystem* jobSystem, const FiberAffinityMask affinityMask, WaitGroup* completionWaitGroup = nullptr,
                      const JobPriority priority = JobPriority::kNormal)
        {
            if (completionWaitGroup)
                m_completionWaitGroup = completionWaitGroup;

            JobScheduleInfo info;
            info.m_job = this;
            info.m_priority = priority;
            info.m_affinityMask = affinityMask;
            jobSystem->Schedule(info);
        }

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
        Rc<WaitGroup> m_completionWaitGroup;
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
