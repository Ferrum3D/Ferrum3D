#pragma once
#include <Core/Jobs/JobNode.h>
#include <Core/Memory/PoolAllocator.h>
#include <Core/Threading/Fiber.h>
#include <Core/Threading/Semaphore.h>
#include <Core/Threading/Thread.h>

namespace FE::Jobs
{
    namespace Internal
    {
        void Init(std::pmr::memory_resource* allocator);
        void Shutdown();
    } // namespace Internal


    struct FiberWaitEntry final : public ConcurrentQueue::Node
    {
        std::atomic<uint64_t> m_orderHint = 0;
        FiberAffinityMask m_affinityMask = FiberAffinityMask::kNone;
        Threading::FiberHandle m_fiber;
        Priority m_priority = Priority::kNormal;
        std::atomic<bool> m_switchCompleted = false;
    };


    struct JobSystem final
    {
        explicit JobSystem();
        ~JobSystem();

        void Init();

        void Start();
        void Stop();

        void Dispatch(JobNode* jobNode);
        FiberAffinityMask GetAffinityMaskForCurrentThread() const;

        static JobSystem& Get();

    private:
        friend struct WaitGroup;

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

        void AddReadyJob(JobNode* job);
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

            ThreadPoolType m_threadPoolType = ThreadPoolType::kGeneric;
            Priority m_priority = Priority::kNormal;
            FiberAffinityMask m_affinityMask = FiberAffinityMask::kNone;
            Threading::Semaphore m_workSemaphore;

            // Jobs in these queues can only be processed by this worker (due to affinity).
            ConcurrentQueue m_jobQueues[festd::to_underlying(Priority::kCount)] = {};
            ConcurrentQueue m_readyFiberQueues[festd::to_underlying(Priority::kCount)] = {};
        };

        struct alignas(Memory::kCacheLineSize) GlobalQueueSet final
        {
            Threading::SpinLock m_consumerLock;
            ConcurrentQueue m_jobQueues[festd::to_underlying(ThreadPoolType::kCount)] = {};
            ConcurrentQueue m_readyFiberQueues[festd::to_underlying(ThreadPoolType::kCount)] = {};
        };

        std::atomic<uint64_t> m_jobCounter = 0;
        GlobalQueueSet m_globalQueues[festd::to_underlying(Priority::kCount)];

        static constexpr uint32_t kMaxWorkerCount = 64;
        festd::array<Worker, kMaxWorkerCount> m_workers;
        Threading::FiberPool m_fiberPool;

        uint32_t m_backgroundWorkerCount = 0;
        uint32_t m_foregroundWorkerCount = 0;

        Threading::Semaphore m_semaphore;
        std::atomic<bool> m_shouldExit = false;
        std::atomic<bool> m_started = false;
        std::atomic<uint32_t> m_wakeIndexAll = 0;
        std::atomic<uint32_t> m_wakeIndexForeground = 0;
        std::atomic<uint32_t> m_wakeIndexBackground = 0;

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

        uint32_t SelectWorkerIndex(ThreadPoolType poolType);
        void SignalWorkForAffinity(FiberAffinityMask affinityMask);

        void ThreadProc(uint32_t workerIndex);
        void FiberProc(Context::TransferParams transferParams);

        static void FiberProcImpl(Context::TransferParams transferParams);
    };
} // namespace FE::Jobs
