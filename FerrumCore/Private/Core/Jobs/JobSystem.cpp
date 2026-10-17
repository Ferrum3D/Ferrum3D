#include <Core/Base/Platform.h>
#include <Core/Jobs/JobSystem.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Jobs/WaitGroup.h>
#include <Core/Strings/Format.h>

namespace FE::Jobs
{
    namespace
    {
        JobSystem* GJobSystem;
        thread_local uint32_t GWorkerIndex = kInvalidIndex;
    } // namespace


    void Internal::Init(std::pmr::memory_resource* allocator)
    {
        FE_Assert(GJobSystem == nullptr, "JobSystem already initialized");
        GJobSystem = Memory::New<JobSystem>(allocator);
        GJobSystem->Init();
    }


    void Internal::Shutdown()
    {
        FE_Assert(GJobSystem != nullptr, "JobSystem not initialized");
        GJobSystem->~JobSystem();
        GJobSystem = nullptr;
    }


    JobSystem& JobSystem::Get()
    {
        return *GJobSystem;
    }


    void JobSystem::AddReadyFiber(FiberWaitEntry* entry)
    {
        const FiberAffinityMask affinityMask = entry->m_affinityMask;
        const Priority priority = entry->m_priority;
        GlobalQueueSet& globalQueueSet = m_globalQueues[festd::to_underlying(priority)];
        {
            std::lock_guard lock(globalQueueSet.m_lock);
            entry->m_orderHint = globalQueueSet.m_orderCounter++;
            globalQueueSet.m_count.store(globalQueueSet.m_count.load(std::memory_order_relaxed) + 1, std::memory_order_release);
            switch (affinityMask)
            {
            case FiberAffinityMask::kAll:
                globalQueueSet.m_readyFiberQueues[festd::to_underlying(ThreadPoolType::kGeneric)].Enqueue(entry);
                break;

            case FiberAffinityMask::kAllForeground:
                globalQueueSet.m_readyFiberQueues[festd::to_underlying(ThreadPoolType::kForeground)].Enqueue(entry);
                break;

            case FiberAffinityMask::kForegroundWorkers:
                globalQueueSet.m_readyFiberQueues[festd::to_underlying(ThreadPoolType::kForegroundWorkers)].Enqueue(entry);
                break;

            case FiberAffinityMask::kAllBackground:
                globalQueueSet.m_readyFiberQueues[festd::to_underlying(ThreadPoolType::kBackground)].Enqueue(entry);
                break;

            case FiberAffinityMask::kNone:
            case FiberAffinityMask::kMainThread:
            default:
                {
                    FE_Assert(Bit::PopCount(festd::to_underlying(affinityMask)) == 1, "Invalid affinity mask");
                    const uint32_t threadIndex = Bit::CountTrailingZeros(festd::to_underlying(affinityMask));

                    Worker& worker = m_workers[threadIndex];
                    worker.m_readyFiberQueues[festd::to_underlying(priority)].Enqueue(entry);
                    break;
                }
            }
        }

        SignalWorkForAffinity(affinityMask);
    }


    uint32_t JobSystem::GetWorkerIndex() const
    {
        FE_Assert(GWorkerIndex != kInvalidIndex, "Thread not found");
        return GWorkerIndex;
    }


    void JobSystem::SignalWorkForAffinity(const FiberAffinityMask affinityMask)
    {
        uint64_t sleepingWorkers = m_sleepingWorkers.load(std::memory_order_acquire);
        while (const uint64_t eligibleWorkers = sleepingWorkers & festd::to_underlying(affinityMask))
        {
            const uint32_t workerIndex = Bit::CountTrailingZeros(eligibleWorkers);
            const uint64_t workerBit = UINT64_C(1) << workerIndex;
            if (m_sleepingWorkers.compare_exchange_weak(sleepingWorkers,
                                                        sleepingWorkers & ~workerBit,
                                                        std::memory_order_acq_rel,
                                                        std::memory_order_acquire))
            {
                m_workers[workerIndex].m_workSemaphore.Release();
                return;
            }
        }
    }


    void JobSystem::WaitForSignal(FiberWaitEntry& entry)
    {
        if (entry.m_state.load(std::memory_order_acquire) == FiberWaitEntry::State::kSignaled)
            return;

        const uint32_t workerIndex = GetWorkerIndex();
        const Worker& worker = m_workers[workerIndex];
        entry.m_priority = worker.m_priority;
        entry.m_affinityMask = worker.m_affinityMask;
        entry.m_fiber = worker.m_currentFiber;
        SwitchFromWaitingFiber(workerIndex, entry);
    }


    void JobSystem::SignalWaitingFiber(FiberWaitEntry& entry)
    {
        const auto previousState = entry.m_state.exchange(FiberWaitEntry::State::kSignaled, std::memory_order_acq_rel);
        FE_Assert(previousState != FiberWaitEntry::State::kSignaled, "Fiber wait signaled twice");
        if (previousState == FiberWaitEntry::State::kSuspended)
            AddReadyFiber(&entry);
    }


    void JobSystem::SwitchFromWaitingFiber(const uint32_t workerIndex, FiberWaitEntry& entry)
    {
        Worker& worker = m_workers[workerIndex];
        FiberWaitEntry* readyEntry = nullptr;
        TakeWork(workerIndex, worker.m_nextJob, readyEntry);

        worker.m_lastWaitEntry = &entry;
        worker.m_prevFiber = worker.m_currentFiber;
        worker.m_currentFiber = readyEntry ? readyEntry->m_fiber : m_fiberPool.Rent(false);

        const Context::TransferParams transferParams =
            m_fiberPool.Switch(worker.m_currentFiber, reinterpret_cast<uintptr_t>(this));
        CleanUpAfterSwitch(transferParams);
    }


    void JobSystem::CleanUpAfterSwitch(const Context::TransferParams transferParams)
    {
        Worker& worker = m_workers[GetWorkerIndex()];
        const Threading::FiberHandle previousFiber = worker.m_prevFiber;
        FiberWaitEntry* entry = worker.m_lastWaitEntry;
        worker.m_prevFiber.Reset();
        worker.m_lastWaitEntry = nullptr;
        m_fiberPool.Update(previousFiber, transferParams.m_contextHandle);

        if (entry)
        {
            // Whichever side arrives last publishes the saved context. Neither side waits for the other.
            const auto previousState = entry->m_state.exchange(FiberWaitEntry::State::kSuspended, std::memory_order_acq_rel);
            if (previousState == FiberWaitEntry::State::kSignaled)
                AddReadyFiber(entry);
        }
        else if (previousFiber)
        {
            m_fiberPool.Return(previousFiber);
        }
    }


    bool JobSystem::TakeWork(const uint32_t workerIndex, JobNode*& job, FiberWaitEntry*& entry, const bool checkEmptyQueues)
    {
        Worker& worker = m_workers[workerIndex];
        const uint32_t poolIndex = festd::to_underlying(worker.m_threadPoolType);
        const bool isForegroundWorker = workerIndex != 0 && worker.m_threadPoolType == ThreadPoolType::kForeground;
        const uint32_t foregroundWorkersIndex = festd::to_underlying(ThreadPoolType::kForegroundWorkers);
        for (int32_t queueIndex = festd::to_underlying(Priority::kHigh); queueIndex >= 0; --queueIndex)
        {
            GlobalQueueSet& queues = m_globalQueues[queueIndex];
            if (!checkEmptyQueues && queues.m_count.load(std::memory_order_acquire) == 0)
                continue;

            std::lock_guard lock(queues.m_lock);
            entry = TakeOldest(worker.m_readyFiberQueues[queueIndex],
                               queues.m_readyFiberQueues[0],
                               queues.m_readyFiberQueues[poolIndex],
                               isForegroundWorker ? &queues.m_readyFiberQueues[foregroundWorkersIndex] : nullptr);
            if (entry)
            {
                queues.m_count.store(queues.m_count.load(std::memory_order_relaxed) - 1, std::memory_order_release);
                worker.m_priority = entry->m_priority;
                worker.m_affinityMask = entry->m_affinityMask;
                return true;
            }

            job = TakeOldest(worker.m_jobQueues[queueIndex],
                             queues.m_jobQueues[0],
                             queues.m_jobQueues[poolIndex],
                             isForegroundWorker ? &queues.m_jobQueues[foregroundWorkersIndex] : nullptr);
            if (job)
            {
                queues.m_count.store(queues.m_count.load(std::memory_order_relaxed) - 1, std::memory_order_release);
                worker.m_priority = job->m_priority;
                worker.m_affinityMask = job->m_affinityMask;
                return true;
            }
        }

        return false;
    }


    FE_FORCE_INLINE void JobSystem::ThreadProc(const uint32_t workerIndex)
    {
        m_semaphore.Acquire();
        GWorkerIndex = workerIndex;
        m_workers[workerIndex].m_threadId = Threading::GetCurrentThreadID();

        const Threading::FiberHandle initialFiber = m_fiberPool.Rent(false);
        m_workers[workerIndex].m_currentFiber = initialFiber;
        m_workers[workerIndex].m_prevFiber.Reset();
        m_fiberPool.Switch(initialFiber, reinterpret_cast<uintptr_t>(this));
    }


    FE_FORCE_INLINE void JobSystem::FiberProc(Context::TransferParams transferParams)
    {
        const uint32_t initialWorkerIndex = GetWorkerIndex();
        if (!m_workers[initialWorkerIndex].m_exitContext)
            m_workers[initialWorkerIndex].m_exitContext = transferParams.m_contextHandle;

        CleanUpAfterSwitch(transferParams);
        while (!m_shouldExit.load(std::memory_order_acquire))
        {
            const uint32_t workerIndex = GetWorkerIndex();
            Worker& worker = m_workers[workerIndex];
            FiberWaitEntry* waitEntry = nullptr;
            JobNode* job = std::exchange(worker.m_nextJob, nullptr);

            if (!job)
            {
                for (uint32_t attempt = 0; attempt < 8; ++attempt)
                {
                    if (TakeWork(workerIndex, job, waitEntry))
                        break;

                    const uint32_t spinCount = Math::Min(1u << attempt, 32u);
                    for (uint32_t spin = 0; spin < spinCount; ++spin)
                        _mm_pause();
                }
            }

            if (!job && !waitEntry)
            {
                const uint64_t workerBit = UINT64_C(1) << workerIndex;
                m_sleepingWorkers.fetch_or(workerBit, std::memory_order_acq_rel);

                // Enqueue and this final check share queue locks, closing the sleep/wakeup race.
                const bool hasWork = TakeWork(workerIndex, job, waitEntry, true);
                if (!hasWork && !m_shouldExit.load(std::memory_order_acquire))
                    worker.m_workSemaphore.Acquire();

                m_sleepingWorkers.fetch_and(~workerBit, std::memory_order_acq_rel);
                if (!hasWork)
                    continue;
            }

            if (waitEntry)
            {
                worker.m_prevFiber = worker.m_currentFiber;
                worker.m_currentFiber = waitEntry->m_fiber;
                worker.m_affinityMask = waitEntry->m_affinityMask;
                transferParams = m_fiberPool.Switch(worker.m_currentFiber, reinterpret_cast<uintptr_t>(this));
                CleanUpAfterSwitch(transferParams);
                continue;
            }

            if (job)
            {
                Rc completionWaitGroup = job->m_completionWaitGroup;
                job->Execute();
                if (completionWaitGroup)
                    completionWaitGroup->Signal();

                continue;
            }
        }

        TracyFiberLeave;
        Context::Switch(m_workers[GetWorkerIndex()].m_exitContext, 0);
    }


    void JobSystem::FiberProcImpl(const Context::TransferParams transferParams)
    {
        const uintptr_t jobSystemAddress = transferParams.m_userData & ((UINT64_C(1) << 48) - 1);
        reinterpret_cast<JobSystem*>(jobSystemAddress)->FiberProc(transferParams);
    }


    JobSystem::JobSystem()
        : m_fiberPool(&FiberProcImpl)
    {
    }


    JobSystem::~JobSystem()
    {
        Stop();

        for (Worker& worker : m_workers)
            Threading::CloseThread(worker.m_thread);
    }


    void JobSystem::Init()
    {
        const Platform::CpuInfo cpuInfo = Platform::GetCpuInfo();
        const uint32_t threadCount = cpuInfo.m_physicalCores < 8 ? cpuInfo.m_logicalCores : cpuInfo.m_physicalCores;
        const uint32_t workerCount = Math::Clamp(threadCount, 4u, kMaxWorkerCount);
        // const uint32_t workerCount = 2;
        const uint32_t foregroundWorkerCount = Math::CeilDivide(workerCount, 2);
        const uint32_t backgroundWorkerCount = workerCount - foregroundWorkerCount;

        GWorkerIndex = 0;
        Worker& mainThread = m_workers[0];
        mainThread.m_threadId = Threading::GetCurrentThreadID();
        mainThread.m_name = "Main Thread";
        mainThread.m_threadPoolType = ThreadPoolType::kForeground;

        for (uint32_t workerIndex = 1; workerIndex < kMaxWorkerCount; ++workerIndex)
        {
            const bool isForeground = workerIndex < foregroundWorkerCount;
            const bool isBackground = workerIndex >= 32 && workerIndex < 32 + backgroundWorkerCount;
            if (!isForeground && !isBackground)
                continue;

            const auto threadName = isForeground ? Fmt::FixedFormat("Foreground Worker {}", workerIndex)
                                                 : Fmt::FixedFormat("Background Worker {}", workerIndex - 32);
            const auto threadFunc = [](const uintptr_t workerIndex) {
                Get().ThreadProc(static_cast<uint32_t>(workerIndex));
            };

            Worker& worker = m_workers[workerIndex];
            worker.m_thread = Threading::CreateThread(threadName, threadFunc, workerIndex);
            worker.m_name = threadName;
            worker.m_threadPoolType = isForeground ? ThreadPoolType::kForeground : ThreadPoolType::kBackground;
        }

        m_foregroundWorkerCount = foregroundWorkerCount;
        m_backgroundWorkerCount = backgroundWorkerCount;
    }


    void JobSystem::Start()
    {
        const Threading::FiberHandle initialFiber = m_fiberPool.Rent(false);
        Worker& mainThread = m_workers[0];
        mainThread.m_currentFiber = initialFiber;
        FE_Assert(mainThread.m_threadId == Threading::GetCurrentThreadID());
        m_started.store(true, std::memory_order_release);
        m_semaphore.Release(m_backgroundWorkerCount + m_foregroundWorkerCount - 1);
        m_fiberPool.Switch(initialFiber, reinterpret_cast<uintptr_t>(this));
    }


    void JobSystem::Stop()
    {
        m_shouldExit.store(true, std::memory_order_release);
        if (!m_started.load(std::memory_order_acquire))
            m_semaphore.Release(m_backgroundWorkerCount + m_foregroundWorkerCount - 1);

        for (uint32_t workerIndex = 0; workerIndex < m_workers.size(); ++workerIndex)
        {
            if (m_workers[workerIndex].m_threadId == 0 && workerIndex != 0)
                continue;

            m_workers[workerIndex].m_workSemaphore.Release();
        }
    }


    void JobSystem::AddReadyJob(JobNode* job)
    {
        const FiberAffinityMask affinityMask = job->m_affinityMask;
        const Priority priority = job->m_priority;
        GlobalQueueSet& globalQueueSet = m_globalQueues[festd::to_underlying(priority)];
        {
            std::lock_guard lock(globalQueueSet.m_lock);
            job->m_orderHint = globalQueueSet.m_orderCounter++;
            globalQueueSet.m_count.store(globalQueueSet.m_count.load(std::memory_order_relaxed) + 1, std::memory_order_release);
            switch (affinityMask)
            {
            case FiberAffinityMask::kAll:
                globalQueueSet.m_jobQueues[festd::to_underlying(ThreadPoolType::kGeneric)].Enqueue(job);
                break;

            case FiberAffinityMask::kAllForeground:
                globalQueueSet.m_jobQueues[festd::to_underlying(ThreadPoolType::kForeground)].Enqueue(job);
                break;

            case FiberAffinityMask::kForegroundWorkers:
                globalQueueSet.m_jobQueues[festd::to_underlying(ThreadPoolType::kForegroundWorkers)].Enqueue(job);
                break;

            case FiberAffinityMask::kAllBackground:
                globalQueueSet.m_jobQueues[festd::to_underlying(ThreadPoolType::kBackground)].Enqueue(job);
                break;

            case FiberAffinityMask::kNone:
            case FiberAffinityMask::kMainThread:
            default:
                {
                    FE_Assert(Bit::PopCount(festd::to_underlying(affinityMask)) == 1, "Invalid affinity mask");
                    const uint32_t threadIndex = Bit::CountTrailingZeros(festd::to_underlying(affinityMask));

                    Worker& worker = m_workers[threadIndex];
                    worker.m_jobQueues[festd::to_underlying(priority)].Enqueue(job);
                    break;
                }
            }
        }

        SignalWorkForAffinity(affinityMask);
    }


    void JobSystem::Dispatch(JobNode* jobNode)
    {
        FE_Assert(jobNode != nullptr);

        const bool dispatchRequested = jobNode->m_dispatchRequested.exchange(true, std::memory_order_acq_rel);
        FE_Assert(!dispatchRequested, "Jobs can only be dispatched once");

        if (jobNode->DependencySatisfied())
            AddReadyJob(jobNode);
    }


    FiberAffinityMask JobSystem::GetAffinityMaskForCurrentThread() const
    {
        return static_cast<FiberAffinityMask>(UINT64_C(1) << GetWorkerIndex());
    }


    void StartJobSystem()
    {
        JobSystem::Get().Start();
    }


    void StopJobSystem()
    {
        JobSystem::Get().Stop();
    }


    FiberAffinityMask GetAffinityMaskForCurrentThread()
    {
        return JobSystem::Get().GetAffinityMaskForCurrentThread();
    }
} // namespace FE::Jobs
