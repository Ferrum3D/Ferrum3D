#pragma once
#include <FeCore/Jobs/IJobSystem.h>
#include <FeCore/Jobs/WaitGroup.h>
#include <FeCore/Memory/Memory.h>
#include <festd/intrusive_list.h>

namespace FE
{
    struct WaitGroup;


    struct Job : public festd::intrusive_list_node
    {
        virtual ~Job() = default;
        virtual void Execute() = 0;

        void Schedule(IJobSystem* pJobSystem, WaitGroup* pCompletionWaitGroup = nullptr,
                      JobPriority priority = JobPriority::kNormal)
        {
            FE_CORE_ASSERT(m_completionWaitGroup == nullptr, "Already scheduled");
            if (pCompletionWaitGroup)
            {
                m_completionWaitGroup = pCompletionWaitGroup;
                pCompletionWaitGroup->Add(1);
            }

            pJobSystem->AddJob(this, priority);
        }

    private:
        friend struct JobSystem;
        Rc<WaitGroup> m_completionWaitGroup;
    };
} // namespace FE
