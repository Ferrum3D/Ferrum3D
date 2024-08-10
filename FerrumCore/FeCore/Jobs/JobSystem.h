#pragma once
#include <FeCore/Jobs/IJobSystem.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Modules/ServiceLocator.h>
#include <FeCore/Parallel/Fiber.h>
#include <FeCore/Parallel/Semaphore.h>
#include <FeCore/Parallel/Thread.h>

namespace FE
{
    struct FiberWaitEntry final : festd::intrusive_list_node
    {
        FiberHandle Fiber;
        JobPriority Priority;
        std::atomic<bool> SwitchCompleted;
    };


    class JobSystem final : public ServiceLocatorImplBase<IJobSystem>
    {
        friend class WaitGroup;

        void* AllocateSmallBlock(size_t byteSize) override;
        void FreeSmallBlock(void* ptr, size_t byteSize) override;

        inline void SwitchFromWaitingFiber(uint32_t workerIndex, FiberWaitEntry& entry)
        {
            Worker& worker = m_Workers[workerIndex];
            worker.pLastWaitEntry = &entry;
            worker.PrevFiber = worker.CurrentFiber;
            worker.CurrentFiber = m_FiberPool.Rent(false);

            const Context::TransferParams tp = m_FiberPool.Switch(worker.CurrentFiber, reinterpret_cast<uintptr_t>(this));
            CleanUpAfterSwitch(tp);
        }

        inline void AddReadyFiber(FiberWaitEntry* pEntry)
        {
            JobQueue& queue = m_JobQueues[enum_cast(pEntry->Priority)];

            std::lock_guard lk{ queue.Lock };
            queue.ReadyFibersQueue.push_back(*pEntry);
        }

        inline void CleanUpAfterSwitch(Context::TransferParams transferParams)
        {
            const uint32_t workerIndex = GetWorkerIndex();
            Worker& worker = m_Workers[workerIndex];
            m_FiberPool.Update(worker.PrevFiber, transferParams.ContextHandle);

            if (worker.pLastWaitEntry)
            {
                worker.pLastWaitEntry->SwitchCompleted.store(true, std::memory_order_release);
                worker.pLastWaitEntry = nullptr;
            }
            else if (worker.PrevFiber)
            {
                FE_CORE_ASSERT(worker.pLastWaitEntry == nullptr, "Wait entry was not cleaned up");
                m_FiberPool.Return(worker.PrevFiber);
            }

            worker.PrevFiber.Reset();
        }

        struct JobQueue final
        {
            SpinLock Lock;
            festd::intrusive_list<Job> Queue;
            festd::intrusive_list<FiberWaitEntry> ReadyFibersQueue;
        };

        struct alignas(Memory::CacheLineSize) Worker final
        {
            uint64_t ThreadID;
            ThreadHandle Thread;
            FiberHandle PrevFiber;
            FiberHandle CurrentFiber;
            FiberWaitEntry* pLastWaitEntry = nullptr;
            Context::Handle ExitContext;
            JobPriority Priority = JobPriority::Normal;
        };

        JobQueue m_JobQueues[enum_cast(JobPriority::Count)];
        festd::fixed_vector<Worker, 64> m_Workers;
        FiberPool m_FiberPool;

        Semaphore m_Semaphore;
        std::atomic<bool> m_ShouldExit;

        inline uint32_t GetWorkerIndex() const
        {
            ZoneScoped;
            const uint64_t threadID = GetCurrentThreadID();
            for (uint32_t threadIndex = 0; threadIndex < m_Workers.size(); ++threadIndex)
            {
                if (m_Workers[threadIndex].ThreadID == threadID)
                    return threadIndex;
            }

            FE_CORE_ASSERT(0, "Thread not found");
            return InvalidIndex;
        }

        void ThreadProc(uint32_t workerIndex);
        void FiberProc(Context::TransferParams transferParams);

        static void FiberProcImpl(Context::TransferParams transferParams);

    public:
        FE_RTTI_Class(JobSystem, "6754DA31-46FA-4661-A46E-2787E6D9FD29");

        explicit JobSystem();
        ~JobSystem() override;

        void Start();
        void Stop();
        void AddJob(Job* pJob, JobPriority priority) override;
    };
} // namespace FE
