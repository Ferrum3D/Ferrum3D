#pragma once
#include <FeCore/Jobs/IJobSystem.h>
#include <FeCore/Jobs/WaitGroup.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/ServiceLocator.h>
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

        inline void Schedule(WaitGroup* pCompletionWaitGroup = nullptr, JobPriority priority = JobPriority::Normal)
        {
            FE_CORE_ASSERT(m_pCompletionWaitGroup == nullptr, "Already scheduled");
            if (pCompletionWaitGroup)
            {
                m_pCompletionWaitGroup = pCompletionWaitGroup;
                pCompletionWaitGroup->Add(1);
            }

            ServiceLocator<IJobSystem>::Get()->AddJob(this, priority);
        }
    };


    template<class TJob>
    struct SmallJob : Job
    {
        ~SmallJob() override = default;

        inline void Execute() final
        {
            TJob* pJob = static_cast<TJob*>(this);
            pJob->DoExecute();
            pJob->~TJob();
            ServiceLocator<IJobSystem>::Get()->FreeSmallBlock(pJob, sizeof(TJob));
        }

        template<class... TArgs>
        inline static TJob* Create(TArgs&&... args)
        {
            static_assert(alignof(TJob) <= Memory::DefaultAlignment);
            void* ptr = ServiceLocator<IJobSystem>::Get()->AllocateSmallBlock(sizeof(TJob));
            return new (ptr) TJob(std::forward<TArgs>(args)...);
        }
    };
} // namespace FE
