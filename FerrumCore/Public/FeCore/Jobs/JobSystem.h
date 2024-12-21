#pragma once
#include <FeCore/Jobs/IJobSystem.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Parallel/Fiber.h>
#include <FeCore/Parallel/Semaphore.h>
#include <FeCore/Parallel/Thread.h>

namespace FE
{
    struct FiberWaitEntry final : festd::intrusive_list_node
    {
        Threading::FiberHandle m_fiber;
        JobPriority m_priority;
        std::atomic<bool> m_switchCompleted;
    };


    struct JobSystem final : public IJobSystem
    {
        FE_RTTI_Class(JobSystem, "6754DA31-46FA-4661-A46E-2787E6D9FD29");

        explicit JobSystem();
        ~JobSystem() override;

        void Start() override;
        void Stop() override;
        void AddJob(Job* pJob, JobPriority priority) override;

    private:
        friend struct WaitGroup;

        void* AllocateSmallBlock(size_t byteSize) override;
        void FreeSmallBlock(void* ptr, size_t byteSize) override;

        void SwitchFromWaitingFiber(uint32_t workerIndex, FiberWaitEntry& entry)
        {
            Worker& worker = m_workers[workerIndex];
            worker.m_lastWaitEntry = &entry;
            worker.m_prevFiber = worker.m_currentFiber;
            worker.m_currentFiber = m_fiberPool.Rent(false);

            const Context::TransferParams tp = m_fiberPool.Switch(worker.m_currentFiber, reinterpret_cast<uintptr_t>(this));
            CleanUpAfterSwitch(tp);
        }

        void AddReadyFiber(FiberWaitEntry* pEntry)
        {
            JobQueue& queue = m_jobQueues[festd::to_underlying(pEntry->m_priority)];

            std::lock_guard lk{ queue.m_lock };
            queue.m_readyFibersQueue.push_back(*pEntry);
        }

        void CleanUpAfterSwitch(Context::TransferParams transferParams)
        {
            const uint32_t workerIndex = GetWorkerIndex();
            Worker& worker = m_workers[workerIndex];
            m_fiberPool.Update(worker.m_prevFiber, transferParams.m_contextHandle);

            if (worker.m_lastWaitEntry)
            {
                worker.m_lastWaitEntry->m_switchCompleted.store(true, std::memory_order_release);
                worker.m_lastWaitEntry = nullptr;
            }
            else if (worker.m_prevFiber)
            {
                FE_CORE_ASSERT(worker.m_lastWaitEntry == nullptr, "Wait entry was not cleaned up");
                m_fiberPool.Return(worker.m_prevFiber);
            }

            worker.m_prevFiber.Reset();
        }

        struct JobQueue final
        {
            Threading::SpinLock m_lock;
            festd::intrusive_list<Job> m_queue;
            festd::intrusive_list<FiberWaitEntry> m_readyFibersQueue;
        };

        struct alignas(Memory::kCacheLineSize) Worker final
        {
            uint64_t m_threadId;
            ThreadHandle m_thread;
            Threading::FiberHandle m_prevFiber;
            Threading::FiberHandle m_currentFiber;
            FiberWaitEntry* m_lastWaitEntry = nullptr;
            Context::Handle m_exitContext;
            JobPriority m_priority = JobPriority::kNormal;
        };

        JobQueue m_jobQueues[festd::to_underlying(JobPriority::kCount)];
        festd::fixed_vector<Worker, 64> m_workers;
        Threading::FiberPool m_fiberPool;

        Threading::Semaphore m_semaphore;
        std::atomic<bool> m_shouldExit;
        std::atomic<bool> m_initialJobPickedUp;

        uint32_t GetWorkerIndex() const
        {
            const uint64_t threadID = GetCurrentThreadID();
            for (uint32_t threadIndex = 0; threadIndex < m_workers.size(); ++threadIndex)
            {
                if (m_workers[threadIndex].m_threadId == threadID)
                    return threadIndex;
            }

            FE_CORE_ASSERT(0, "Thread not found");
            return kInvalidIndex;
        }

        void ThreadProc(uint32_t workerIndex);
        void FiberProc(Context::TransferParams transferParams);

        static void FiberProcImpl(Context::TransferParams transferParams);
    };
} // namespace FE
