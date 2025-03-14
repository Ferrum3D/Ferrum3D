#pragma once
#include <FeCore/Jobs/IJobSystem.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Threading/Fiber.h>
#include <FeCore/Threading/Semaphore.h>
#include <FeCore/Threading/Thread.h>
#include <festd/vector.h>

namespace FE
{
    struct FiberWaitEntry final : public ConcurrentQueueNode
    {
        FiberWaitEntry* m_queueTail = nullptr;
        std::atomic<uint64_t> m_orderHint = 0;
        FiberAffinityMask m_affinityMask = FiberAffinityMask::kNone;
        Threading::FiberHandle m_fiber;
        JobPriority m_priority = JobPriority::kNormal;
        std::atomic<bool> m_switchCompleted = false;
    };


    struct JobSystem final : public IJobSystem
    {
        FE_RTTI_Class(JobSystem, "6754DA31-46FA-4661-A46E-2787E6D9FD29");

        explicit JobSystem();
        ~JobSystem() override;

        void Start() override;
        void Stop() override;
        void Schedule(const JobScheduleInfo& info) override;
        FiberAffinityMask GetAffinityMaskForCurrentThread() const override;

    private:
        friend struct WaitGroup;

        std::pmr::memory_resource* GetWaitGroupAllocator() override;

        void SwitchFromWaitingFiber(const uint32_t workerIndex, FiberWaitEntry& entry)
        {
            Worker& worker = m_workers[workerIndex];
            worker.m_lastWaitEntry = &entry;
            worker.m_prevFiber = worker.m_currentFiber;
            worker.m_currentFiber = m_fiberPool.Rent(false);

            const char* switchMessage = worker.m_name.c_str();
            const Context::TransferParams tp =
                m_fiberPool.Switch(worker.m_currentFiber, reinterpret_cast<uintptr_t>(this), switchMessage);
            CleanUpAfterSwitch(tp);
        }

        void AddReadyFiber(FiberWaitEntry* entry);

        void CleanUpAfterSwitch(const Context::TransferParams transferParams)
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
                FE_Assert(worker.m_lastWaitEntry == nullptr, "Wait entry was not cleaned up");
                m_fiberPool.Return(worker.m_prevFiber);
            }

            worker.m_prevFiber.Reset();
        }

        struct alignas(Memory::kCacheLineSize) Worker final
        {
            uint64_t m_threadId = 0;
            Threading::ThreadHandle m_thread;
            festd::fixed_string m_name;
            Context::Handle m_exitContext;

            Threading::FiberHandle m_prevFiber;
            Threading::FiberHandle m_currentFiber;
            FiberWaitEntry* m_lastWaitEntry = nullptr;

            JobThreadPoolType m_threadPoolType = JobThreadPoolType::kGeneric;
            JobPriority m_priority = JobPriority::kNormal;
            FiberAffinityMask m_affinityMask = FiberAffinityMask::kNone;

            // Jobs in these queues can only be processed by this worker (due to affinity).
            ConcurrentQueue m_jobQueues[festd::to_underlying(JobPriority::kCount)] = {};
            ConcurrentQueue m_readyFiberQueues[festd::to_underlying(JobPriority::kCount)] = {};
        };

        struct alignas(Memory::kCacheLineSize) GlobalQueueSet final
        {
            Threading::SpinLock m_consumerLocks[festd::to_underlying(JobThreadPoolType::kCount)] = {};
            ConcurrentQueue m_jobQueues[festd::to_underlying(JobThreadPoolType::kCount)] = {};
            ConcurrentQueue m_readyFiberQueues[festd::to_underlying(JobThreadPoolType::kCount)] = {};
        };

        std::atomic<uint64_t> m_jobCounter = 0;
        GlobalQueueSet m_globalQueues[festd::to_underlying(JobPriority::kCount)];

        static constexpr uint32_t kMaxWorkerCount = 64;
        festd::array<Worker, kMaxWorkerCount> m_workers;
        Threading::FiberPool m_fiberPool;
        Memory::PoolAllocator m_waitGroupPool;

        uint32_t m_backgroundWorkerCount = 0;
        uint32_t m_foregroundWorkerCount = 0;

        Threading::Semaphore m_semaphore;
        std::atomic<bool> m_shouldExit;

        uint32_t GetWorkerIndex() const
        {
            const uint64_t threadID = Threading::GetCurrentThreadID();
            for (uint32_t threadIndex = 0; threadIndex < m_workers.size(); ++threadIndex)
            {
                if (m_workers[threadIndex].m_threadId == threadID)
                    return threadIndex;
            }

            FE_Assert(0, "Thread not found");
            return kInvalidIndex;
        }

        void ThreadProc(uint32_t workerIndex);
        void FiberProc(Context::TransferParams transferParams);

        static void FiberProcImpl(Context::TransferParams transferParams);
    };
} // namespace FE
