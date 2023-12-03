#pragma once
#include <FeCore/Jobs/IJobScheduler.h>
#include <FeCore/Jobs/JobTree.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/ServiceLocator.h>
#include <atomic>

namespace FE
{
    //! \brief Execution state of job.
    //!
    //! \note Every value of this enum must fit in 2 bits.
    enum class JobExecutionState
    {
        NotReady,  //!< The job is waiting for dependencies or initializing.
        Pending,   //!< The job is in a queue waiting to be executed by a worker.
        Execution, //!< The job is being executed by a worker thread.
        Complete   //!< The job has completed execution.
    };

    //! \brief Priority of a job: from Low to Highest.
    //!
    //! \note Every value of this enum must fit in 2 bits.
    enum class JobPriority
    {
        Low,
        Normal,
        High,
        Highest
    };

    struct JobExecutionContext
    {
        FE_STRUCT_RTTI(JobExecutionContext, "F1295370-E5FC-4D4B-B657-7A0158F2D22C");

        UInt32 WorkerID;
    };

    //! \brief A unit of work that can be processed quickly on one thread.
    //!
    //! Multiple jobs can be scheduled and processed in parallel. If a job depends on results of other jobs,
    //! it can be set as dependent of these jobs. The job stores its dependency counter which decreases when
    //! a parent job completes. When the counter reaches zero, the job is pushed onto the global queue.
    //!
    //! The default value of dependency counter is 1, so the job won't start immediately, but will wait for
    //! a call to `Schedule()` that will decrement the counter.
    //!
    //! Jobs aren't managed with ref-counters to reduce overhead. The best way is to reuse jobs every frame
    //! and store them as class fields:
    //!
    //! \code{.cpp}
    //!     class MyJob : public FE::Job
    //!     {
    //!     public:
    //!         void Execute(const FE::JobExecutionContext& context) override {/* Execute job */ }
    //!     };
    //!     class MyGameClass : public FE::EventBus<FE::FrameEvents>::Handler
    //!     {
    //!         MyJob m_Job;
    //!     public:
    //!         void OnUpdate() override { /* Reset and schedule m_Job */ }
    //!     };
    //! \endcode
    //!
    //! In this case the owner of the job must live long enough to allow the job to complete. But there's
    //! another way to schedule a job: to allocate it with job scheduler's monotonic allocator and let the
    //! scheduler automatically delete it in the next one or two frames. To do that you must be sure that
    //! your job's data is stored entirely in the job class, so we can just reset the scheduler's allocator
    //! without calling every job's destructor. Also these jobs must be completed in time of a single frame.
    class Job
    {
        std::atomic<UInt16> m_Flags{};

        // There will probably be a lot of small jobs in the engine, so I choose too pack bits together
        // and reduce size of job's data as much as possible.

        inline static constexpr UInt16 PriorityBitCount        = 2;
        inline static constexpr UInt16 StateBitCount           = 2;
        inline static constexpr UInt16 DependencyCountBitCount = 16 - PriorityBitCount - StateBitCount;

        inline static constexpr UInt16 DependencyCountShift = 0;
        inline static constexpr UInt16 PriorityShift        = DependencyCountBitCount;
        inline static constexpr UInt16 StateShift           = PriorityShift + PriorityBitCount;

        inline static constexpr UInt16 DependencyCountMask = MakeMask(DependencyCountBitCount, DependencyCountShift);
        inline static constexpr UInt16 PriorityMask        = MakeMask(PriorityBitCount, PriorityShift);
        inline static constexpr UInt16 StateMask           = MakeMask(StateBitCount, StateShift);

        inline void SetExecutionState(JobExecutionState state);

        inline UInt16 IncrementDependencyCount();
        inline UInt16 DecrementDependencyCount();

    protected:
        BoolPointer<JobTree> m_TreeEmptyPair;
        Job* m_Dependent = nullptr;

        //! \brief Synchronously run the job. Will be called from worker thread.
        //!
        //! \param [in] context - The job's context used for execution.
        virtual void Execute(const JobExecutionContext& context) = 0;

    public:
        FE_CLASS_RTTI(Job, "69DA12B5-DFFC-4A38-BBB8-0018699C30BA");

        inline explicit Job(JobPriority priority = JobPriority::Normal, bool isEmpty = false);
        ~Job() = default;

        inline void ExecuteInternal(const JobExecutionContext& context);

        //! \brief Attach job to a JobTree node.
        //!
        //! \param [in] tree - The node of JobTree to attach the job to.
        inline void AttachToTree(JobTree* tree);

        //! \brief Add a parent job to depend on it.
        //!
        //! This function will increment dependency counter and the parent will decrement it when it completes.
        //! A job can have multiple parents and won't be scheduled until all the parents signaled they're done.
        //!
        //! \param [in] parent - Parent job to attach.
        inline void AttachParent(Job* parent);

        //! \brief Schedule the job to global scheduler.
        //!
        //! This function will decrement dependency counter. If job doesn't have dependencies, it will be scheduled
        //! immediately. If however there are uncompleted parent jobs, actual scheduling will occur on scheduler
        //! only when dependency counter reaches zero.
        inline void Schedule();

        inline void Complete();

        //! \return Job's priority.
        [[nodiscard]] inline JobPriority GetPriority() const;

        //! \brief Set job's priority.
        //!
        //! \param [in] priority - Priority to set for this job.
        inline void SetPriority(JobPriority priority);

        //! \return Job's execution state.
        [[nodiscard]] inline JobExecutionState GetExecutionState() const;

        [[nodiscard]] inline bool Empty() const;

        //! \return Maximum number of dependencies a job can handle.
        [[nodiscard]] static inline constexpr UInt16 GetMaxDependencyCount()
        {
            return DependencyCountMask >> DependencyCountShift;
        }
    };

    Job::Job(JobPriority priority, bool isEmpty)
        : m_TreeEmptyPair(nullptr, isEmpty)
    {
        UInt16 value = static_cast<UInt16>(priority) << PriorityShift;
        value |= 1 << DependencyCountShift;
        value |= static_cast<UInt16>(JobExecutionState::NotReady) << StateShift;
        m_Flags.store(value, std::memory_order_relaxed);
    }

    void Job::AttachToTree(JobTree* tree)
    {
        m_TreeEmptyPair.SetPointer(tree);
    }

    void Job::AttachParent(Job* parent)
    {
        parent->m_Dependent = this;
        IncrementDependencyCount();
    }

    void Job::SetExecutionState(JobExecutionState state)
    {
        UInt16 initial = m_Flags.load();
        UInt16 value   = static_cast<UInt16>(state) << StateShift;
        while (!m_Flags.compare_exchange_weak(initial, value | (initial & ~StateMask)));
    }

    JobExecutionState Job::GetExecutionState() const
    {
        UInt16 value = m_Flags.load(std::memory_order_relaxed) & StateMask;
        return static_cast<JobExecutionState>(value >> StateShift);
    }

    void Job::SetPriority(JobPriority priority)
    {
        UInt16 initial = m_Flags.load();
        UInt16 value   = static_cast<UInt16>(priority) << PriorityShift;
        while (!m_Flags.compare_exchange_weak(initial, value | (initial & ~PriorityMask)));
    }

    JobPriority Job::GetPriority() const
    {
        UInt16 value = m_Flags.load(std::memory_order_relaxed) & PriorityMask;
        return static_cast<JobPriority>(value >> PriorityShift);
    }

    void Job::Schedule()
    {
        DecrementDependencyCount();
    }

    UInt16 Job::IncrementDependencyCount()
    {
        return (++m_Flags & DependencyCountMask) >> DependencyCountShift;
    }

    UInt16 Job::DecrementDependencyCount()
    {
        auto value = (--m_Flags & DependencyCountMask) >> DependencyCountShift;
        if (value == 0)
        {
            SetExecutionState(JobExecutionState::Pending);
            ServiceLocator<IJobScheduler>::Get()->ScheduleJob(this);
        }
        return static_cast<UInt16>(value);
    }

    void Job::ExecuteInternal(const JobExecutionContext& context)
    {
        SetExecutionState(JobExecutionState::Execution);
        Execute(context);
        SetExecutionState(JobExecutionState::Complete);
        if (m_Dependent)
        {
            m_Dependent->DecrementDependencyCount();
        }
    }

    bool Job::Empty() const
    {
        return m_TreeEmptyPair.GetBool();
    }

    void Job::Complete()
    {
        Schedule();
        ServiceLocator<IJobScheduler>::Get()->SuspendUntilComplete(this);
    }
} // namespace FE
