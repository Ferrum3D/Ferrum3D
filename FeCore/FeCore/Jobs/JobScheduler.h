#pragma once
#include <FeCore/Allocators/MonotonicAllocator.h>
#include <FeCore/EventBus/FrameEvents.h>
#include <FeCore/Jobs/IJobScheduler.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Modules/Singleton.h>
#include <FeCore/Parallel/Semaphore.h>
#include <FeCore/EventBus/EventBus.h>
#include <deque>
#include <thread>

namespace FE
{
    class JobGlobalQueue
    {
        using DequeAllocator = StdHeapAllocator<Job*>;
        std::deque<Job*, DequeAllocator> m_Deque;
        Mutex m_Mutex;

    public:
        inline bool Empty();
        inline void Enqueue(Job* job);
        inline Job* Dequeue();
    };

    bool JobGlobalQueue::Empty()
    {
        Locker lk(m_Mutex);
        return m_Deque.empty();
    }

    void JobGlobalQueue::Enqueue(Job* job)
    {
        Locker lk(m_Mutex);
        constexpr auto compare = [](Job* lhs, Job* rhs) {
            return lhs->GetPriority() > rhs->GetPriority();
        };

        auto it = std::lower_bound(m_Deque.begin(), m_Deque.end(), job, compare);
        m_Deque.insert(it, job);
    }

    Job* JobGlobalQueue::Dequeue()
    {
        Locker lk(m_Mutex);
        if (!m_Deque.empty())
        {
            auto* t = m_Deque.front();
            m_Deque.pop_front();
            return t;
        }
        return nullptr;
    }

    class JobWorkerQueue
    {
        using DequeAllocator = StdHeapAllocator<Job*>;
        std::deque<Job*, DequeAllocator> m_Deque;
        Mutex m_Mutex;

        inline Job* GetFrontNoLock();

    public:
        inline void Enqueue(Job* job);
        inline Job* SelfSteal();
        inline Job* Steal();
    };

    Job* JobWorkerQueue::GetFrontNoLock()
    {
        if (!m_Deque.empty())
        {
            auto* front = m_Deque.front();
            m_Deque.pop_front();
            return front;
        }

        return nullptr;
    }

    void JobWorkerQueue::Enqueue(Job* job)
    {
        Locker lk(m_Mutex);
        constexpr auto compare = [](Job* lhs, Job* rhs) {
            return lhs->GetPriority() > rhs->GetPriority();
        };

        auto it = std::lower_bound(m_Deque.begin(), m_Deque.end(), job, compare);
        m_Deque.insert(it, job);
    }

    Job* JobWorkerQueue::SelfSteal()
    {
        Locker lk(m_Mutex);
        return GetFrontNoLock();
    }

    Job* JobWorkerQueue::Steal()
    {
        auto pauseCount = 1;
        for (Int32 i = 0; i < 16; ++i)
        {
            if (m_Mutex.TryLock())
            {
                auto* front = GetFrontNoLock();
                m_Mutex.Unlock();
                return front;
            }
            if (pauseCount > 32)
            {
                std::this_thread::yield();
                continue;
            }
            for (Int32 j = 0; j < pauseCount; ++j)
            {
                _mm_pause();
            }
            pauseCount <<= 1;
        }
        return nullptr;
    }

    struct SchedulerThreadInfo
    {
        std::thread Thread;
        JobWorkerQueue Queue;
        UInt32 WorkerID = static_cast<UInt32>(-1);
        std::thread::id ThreadID;
        Semaphore WaitSemaphore;
        AtomicInt32 IsSleeping;

        [[nodiscard]] inline bool IsWorker() const noexcept
        {
            return WorkerID != static_cast<UInt32>(-1);
        }
    };

    class JobScheduler final
        : public SingletonImplBase<IJobScheduler>
        , public EventBus<FrameEvents>::Handler
    {
        const UInt32 m_WorkerCount;
        Vector<SchedulerThreadInfo*> m_Threads;
        JobGlobalQueue m_GlobalQueue;

        Semaphore m_Semaphore;
        AtomicInt32 m_SleepingWorkerCount;
        AtomicInt32 m_ShouldExit;

        AtomicInt32 m_FrameIndex;
        MonotonicAllocatorAsync m_OneFrameAllocators[2];
        MonotonicAllocatorSync m_ThreadInfoAllocator;

        static thread_local SchedulerThreadInfo* m_CurrentThreadInfo;
        inline static constexpr UInt32 MaxThreadCount = 32;

        void NotifyWorker();
        void WorkerThreadProcess(UInt32 id);
        static void Execute(Job* job);
        SchedulerThreadInfo* GetCurrentThread();
        Job* TryStealJob(UInt32& victimIndex);

    public:
        FE_CLASS_RTTI(JobScheduler, "6754DA31-46FA-4661-A46E-2787E6D9FD29");

        explicit JobScheduler(UInt32 workerCount);
        ~JobScheduler() noexcept override;

        [[nodiscard]] UInt32 GetWorkerCount() const override;
        [[nodiscard]] UInt32 GetWorkerID() const override;

        void* OneFrameAllocate(USize size, USize alignment) override;

        void ScheduleJob(Job* job) override;

        void OnFrameStart(const FrameEventArgs& args) override;
    };
} // namespace FE
