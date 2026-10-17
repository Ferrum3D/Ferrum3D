#pragma once
#include <Core/Jobs/JobNode.h>
#include <Core/Memory/PoolAllocator.h>
#include <Core/Threading/Fiber.h>
#include <Core/Threading/Semaphore.h>
#include <Core/Threading/Thread.h>

namespace FE::Threading
{
    struct FiberMutex;
}

namespace FE::Jobs
{
    namespace Internal
    {
        void Init(std::pmr::memory_resource* allocator);
        void Shutdown();
    } // namespace Internal


    struct FiberWaitEntry final : public ConcurrentQueue::Node
    {
        enum class State : uint8_t
        {
            kWaiting,
            kSuspended,
            kSignaled,
        };

        uint64_t m_orderHint = 0;
        FiberAffinityMask m_affinityMask = FiberAffinityMask::kNone;
        Threading::FiberHandle m_fiber;
        Priority m_priority = Priority::kNormal;
        std::atomic<State> m_state = State::kWaiting;
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
        friend struct Threading::FiberMutex;

        void WaitForSignal(FiberWaitEntry& entry);
        void SignalWaitingFiber(FiberWaitEntry& entry);
        void SwitchFromWaitingFiber(uint32_t workerIndex, FiberWaitEntry& entry);
        void AddReadyFiber(FiberWaitEntry* entry);
        void AddReadyJob(JobNode* job);
        void CleanUpAfterSwitch(Context::TransferParams transferParams);
        bool TakeWork(uint32_t workerIndex, JobNode*& job, FiberWaitEntry*& entry, bool checkEmptyQueues = false);

        // All queues at a priority share its lock, including producer access.
        template<class T>
        struct RunQueue final
        {
            T* m_head = nullptr;
            T* m_tail = nullptr;

            void Enqueue(T* node)
            {
                node->m_next = nullptr;
                if (m_tail)
                    m_tail->m_next = node;
                else
                    m_head = node;

                m_tail = node;
            }


            T* Dequeue()
            {
                T* node = m_head;
                m_head = static_cast<T*>(node->m_next);
                if (!m_head)
                    m_tail = nullptr;

                return node;
            }
        };


        template<class T>
        static T* TakeOldest(RunQueue<T>& local, RunQueue<T>& generic, RunQueue<T>& pool, RunQueue<T>* foregroundWorkers)
        {
            RunQueue<T>* selected = &local;
            if (generic.m_head && (!selected->m_head || generic.m_head->m_orderHint < selected->m_head->m_orderHint))
                selected = &generic;

            if (pool.m_head && (!selected->m_head || pool.m_head->m_orderHint < selected->m_head->m_orderHint))
                selected = &pool;

            if (foregroundWorkers && foregroundWorkers->m_head)
            {
                if (!selected->m_head || foregroundWorkers->m_head->m_orderHint < selected->m_head->m_orderHint)
                    selected = foregroundWorkers;
            }

            return selected->m_head ? selected->Dequeue() : nullptr;
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
            JobNode* m_nextJob = nullptr;

            ThreadPoolType m_threadPoolType = ThreadPoolType::kGeneric;
            Priority m_priority = Priority::kNormal;
            FiberAffinityMask m_affinityMask = FiberAffinityMask::kNone;
            Threading::Semaphore m_workSemaphore;

            // Jobs in these queues can only be processed by this worker (due to affinity).
            RunQueue<JobNode> m_jobQueues[festd::to_underlying(Priority::kCount)] = {};
            RunQueue<FiberWaitEntry> m_readyFiberQueues[festd::to_underlying(Priority::kCount)] = {};
        };

        struct alignas(Memory::kCacheLineSize) GlobalQueueSet final
        {
            Threading::SpinLock m_lock;
            uint64_t m_orderCounter = 0;
            std::atomic<uint32_t> m_count = 0;
            RunQueue<JobNode> m_jobQueues[festd::to_underlying(ThreadPoolType::kCount)] = {};
            RunQueue<FiberWaitEntry> m_readyFiberQueues[festd::to_underlying(ThreadPoolType::kCount)] = {};
        };

        GlobalQueueSet m_globalQueues[festd::to_underlying(Priority::kCount)];

        static constexpr uint32_t kMaxWorkerCount = 64;
        festd::array<Worker, kMaxWorkerCount> m_workers;
        Threading::FiberPool m_fiberPool;

        uint32_t m_backgroundWorkerCount = 0;
        uint32_t m_foregroundWorkerCount = 0;

        Threading::Semaphore m_semaphore;
        std::atomic<bool> m_shouldExit = false;
        std::atomic<bool> m_started = false;
        std::atomic<uint64_t> m_sleepingWorkers = 0;

        // A noinline TLS lookup must be repeated after a fiber migrates to another thread.
        FE_FORCE_NOINLINE uint32_t GetWorkerIndex() const;

        void SignalWorkForAffinity(FiberAffinityMask affinityMask);

        void ThreadProc(uint32_t workerIndex);
        void FiberProc(Context::TransferParams transferParams);

        static void FiberProcImpl(Context::TransferParams transferParams);
    };
} // namespace FE::Jobs
