#pragma once
#include <FeCore/Jobs/IJobSystem.h>
#include <FeCore/Jobs/WaitGroup.h>
#include <FeCore/Memory/Memory.h>
#include <atomic>

namespace FE
{
    class WaitGroup;


    class Job : public festd::intrusive_list_node
    {
        friend class JobSystem;
        Rc<WaitGroup> m_pCompletionWaitGroup;

    public:
        virtual ~Job() = default;
        virtual void Execute() = 0;

        inline void Schedule(IJobSystem* pJobSystem, WaitGroup* pCompletionWaitGroup = nullptr,
                             JobPriority priority = JobPriority::kNormal)
        {
            FE_CORE_ASSERT(m_pCompletionWaitGroup == nullptr, "Already scheduled");
            if (pCompletionWaitGroup)
            {
                m_pCompletionWaitGroup = pCompletionWaitGroup;
                pCompletionWaitGroup->Add(1);
            }

            pJobSystem->AddJob(this, priority);
        }
    };
} // namespace FE
