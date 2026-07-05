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
    }


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
        entry->m_orderHint = m_jobCounter.fetch_add(1, std::memory_order_relaxed);

        const FiberAffinityMask affinityMask = entry->m_affinityMask;
        const Priority priority = entry->m_priority;
        GlobalQueueSet& globalQueueSet = m_globalQueues[festd::to_underlying(priority)];
        switch (affinityMask)
        {
        case FiberAffinityMask::kAll:
            globalQueueSet.m_readyFiberQueues[festd::to_underlying(ThreadPoolType::kGeneric)].Enqueue(entry);
            break;

        case FiberAffinityMask::kAllForeground:
            globalQueueSet.m_readyFiberQueues[festd::to_underlying(ThreadPoolType::kForeground)].Enqueue(entry);
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

        SignalWorkForAffinity(affinityMask);
    }


    uint32_t JobSystem::SelectWorkerIndex(const ThreadPoolType poolType)
    {
        switch (poolType)
        {
        case ThreadPoolType::kForeground:
            return m_foregroundWorkerCount == 0
                ? 0
                : m_wakeIndexForeground.fetch_add(1, std::memory_order_relaxed) % m_foregroundWorkerCount;

        case ThreadPoolType::kBackground:
            return m_backgroundWorkerCount == 0
                ? 0
                : 32 + (m_wakeIndexBackground.fetch_add(1, std::memory_order_relaxed) % m_backgroundWorkerCount);

        case ThreadPoolType::kGeneric:
        default:
            {
                const uint32_t totalWorkers = m_foregroundWorkerCount + m_backgroundWorkerCount;
                if (totalWorkers == 0)
                    return 0;

                const uint32_t index = m_wakeIndexAll.fetch_add(1, std::memory_order_relaxed) % totalWorkers;
                if (index < m_foregroundWorkerCount)
                    return index;

                return 32 + (index - m_foregroundWorkerCount);
            }
        }
    }


    void JobSystem::SignalWorkForAffinity(const FiberAffinityMask affinityMask)
    {
        switch (affinityMask)
        {
        case FiberAffinityMask::kAll:
            m_workers[SelectWorkerIndex(ThreadPoolType::kGeneric)].m_workSemaphore.Release();
            break;
        case FiberAffinityMask::kAllForeground:
            m_workers[SelectWorkerIndex(ThreadPoolType::kForeground)].m_workSemaphore.Release();
            break;
        case FiberAffinityMask::kAllBackground:
            m_workers[SelectWorkerIndex(ThreadPoolType::kBackground)].m_workSemaphore.Release();
            break;
        case FiberAffinityMask::kMainThread:
            m_workers[0].m_workSemaphore.Release();
            break;
        case FiberAffinityMask::kNone:
        default:
            {
                FE_Assert(Bit::PopCount(festd::to_underlying(affinityMask)) == 1, "Invalid affinity mask");
                const uint32_t threadIndex = Bit::CountTrailingZeros(festd::to_underlying(affinityMask));
                m_workers[threadIndex].m_workSemaphore.Release();
                break;
            }
        }
    }


    FE_FORCE_INLINE void JobSystem::ThreadProc(const uint32_t workerIndex)
    {
        m_semaphore.Acquire();
        m_workers[workerIndex].m_threadId = Threading::GetCurrentThreadID();

        const Threading::FiberHandle initialFiber = m_fiberPool.Rent(false);
        m_workers[workerIndex].m_currentFiber = initialFiber;
        m_workers[workerIndex].m_prevFiber.Reset();
        m_fiberPool.Switch(initialFiber, reinterpret_cast<uintptr_t>(this), m_workers[workerIndex].m_name.c_str());
    }


    FE_FORCE_INLINE void JobSystem::FiberProc(Context::TransferParams transferParams)
    {
        const uint32_t initialWorkerIndex = GetWorkerIndex();
        if (!m_workers[initialWorkerIndex].m_exitContext)
            m_workers[initialWorkerIndex].m_exitContext = transferParams.m_contextHandle;

        CleanUpAfterSwitch(transferParams);
        while (!m_shouldExit.load(std::memory_order_acquire))
        {
            Worker& worker = m_workers[GetWorkerIndex()];

            const FiberWaitEntry* waitEntry = nullptr;
            JobNode* job = nullptr;
            auto affinityMask = FiberAffinityMask::kNone;

            for (uint32_t attempt = 0; attempt < 8; ++attempt)
            {
                for (int32_t queueIndex = festd::to_underlying(Priority::kHigh); queueIndex >= 0; --queueIndex)
                {
                    GlobalQueueSet& globalQueueSet = m_globalQueues[queueIndex];

                    // Firstly, try to continue pending fiber.

                    FiberWaitEntry* fibers[3] = {};
                    ConcurrentQueue* queues[3] = {};

                    queues[0] = &worker.m_readyFiberQueues[queueIndex];
                    queues[1] = &globalQueueSet.m_readyFiberQueues[festd::to_underlying(ThreadPoolType::kGeneric)];
                    queues[2] = &globalQueueSet.m_readyFiberQueues[festd::to_underlying(worker.m_threadPoolType)];

                    // Since we use single-consumer queues, and we might push a job back to the front of the queue,
                    // we need this lock here.

                    std::unique_lock lock{ globalQueueSet.m_consumerLock };

                    for (uint32_t i = 0; i < 3; ++i)
                        fibers[i] = static_cast<FiberWaitEntry*>(queues[i]->TryDequeue());

                    // We can potentially have three fibers: one from the local queue, and two from the global queues.
                    // Check which one has been scheduled earlier.

                    uint32_t fiberIndex = kInvalidIndex;
                    uint64_t fiberOrderHint = Constants::kMaxU64;
                    for (uint32_t i = 0; i < 3; ++i)
                    {
                        if (fibers[i] && fibers[i]->m_orderHint < fiberOrderHint)
                        {
                            fiberIndex = i;
                            fiberOrderHint = fibers[i]->m_orderHint;
                        }
                    }

                    for (uint32_t i = 0; i < 3; ++i)
                    {
                        // Push the rest of the fibers back to the front of the queue.
                        if (i != fiberIndex && fibers[i])
                            queues[i]->PushFront(fibers[i]);
                    }

                    if (fiberIndex != kInvalidIndex)
                    {
                        worker.m_priority = static_cast<Priority>(queueIndex);
                        waitEntry = fibers[fiberIndex];

                        switch (fiberIndex)
                        {
                        case 0:
                            affinityMask = static_cast<FiberAffinityMask>(UINT64_C(1) << GetWorkerIndex());
                            break;
                        case 1:
                            affinityMask = FiberAffinityMask::kAll;
                            break;
                        case 2:
                            affinityMask = worker.m_threadPoolType == ThreadPoolType::kForeground
                                ? FiberAffinityMask::kAllForeground
                                : FiberAffinityMask::kAllBackground;
                            break;
                        default:
                            FE_DebugBreak();
                            break;
                        }

                        break;
                    }

                    // If we don't have any ready fibers, try to get a new job.
                    // The same logic applies here.

                    JobNode* jobs[3] = {};

                    queues[0] = &worker.m_jobQueues[queueIndex];
                    queues[1] = &globalQueueSet.m_jobQueues[festd::to_underlying(ThreadPoolType::kGeneric)];
                    queues[2] = &globalQueueSet.m_jobQueues[festd::to_underlying(worker.m_threadPoolType)];

                    for (uint32_t i = 0; i < 3; ++i)
                        jobs[i] = static_cast<JobNode*>(queues[i]->TryDequeue());

                    uint32_t jobIndex = kInvalidIndex;
                    uint64_t jobOrderHint = Constants::kMaxU64;
                    for (uint32_t i = 0; i < 3; ++i)
                    {
                        if (jobs[i] && jobs[i]->m_orderHint < jobOrderHint)
                        {
                            jobIndex = i;
                            jobOrderHint = jobs[i]->m_orderHint;
                        }
                    }

                    for (uint32_t i = 0; i < 3; ++i)
                    {
                        if (i != jobIndex && jobs[i])
                            queues[i]->PushFront(jobs[i]);
                    }

                    if (jobIndex != kInvalidIndex)
                    {
                        worker.m_priority = static_cast<Priority>(queueIndex);
                        job = jobs[jobIndex];

                        switch (jobIndex)
                        {
                        case 0:
                            affinityMask = static_cast<FiberAffinityMask>(UINT64_C(1) << GetWorkerIndex());
                            break;
                        case 1:
                            affinityMask = FiberAffinityMask::kAll;
                            break;
                        case 2:
                            affinityMask = worker.m_threadPoolType == ThreadPoolType::kForeground
                                ? FiberAffinityMask::kAllForeground
                                : FiberAffinityMask::kAllBackground;
                            break;
                        default:
                            FE_DebugBreak();
                            break;
                        }

                        break;
                    }
                }

                if (job || waitEntry)
                    break;

                const uint32_t spinCount = Math::Min(1u << attempt, 32u);
                for (uint32_t spin = 0; spin < spinCount; ++spin)
                    _mm_pause();
            }

            if (waitEntry)
            {
                worker.m_prevFiber = worker.m_currentFiber;
                worker.m_currentFiber = waitEntry->m_fiber;
                worker.m_affinityMask = waitEntry->m_affinityMask;
                const char* switchMessage = worker.m_name.c_str();
                transferParams = m_fiberPool.Switch(worker.m_currentFiber, reinterpret_cast<uintptr_t>(this), switchMessage);
                CleanUpAfterSwitch(transferParams);
                continue;
            }

            if (job)
            {
                worker.m_affinityMask = affinityMask;

                Rc completionWaitGroup = job->m_completionWaitGroup;
                job->Execute();
                if (completionWaitGroup)
                    completionWaitGroup->Signal();

                continue;
            }

            if (m_shouldExit.load(std::memory_order_acquire))
                continue;

            worker.m_workSemaphore.Acquire();
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
        m_fiberPool.Switch(initialFiber, reinterpret_cast<uintptr_t>(this), mainThread.m_name.c_str());
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
        job->m_orderHint = m_jobCounter.fetch_add(1, std::memory_order_relaxed);

        const FiberAffinityMask affinityMask = job->m_affinityMask;
        const Priority priority = job->m_priority;
        GlobalQueueSet& globalQueueSet = m_globalQueues[festd::to_underlying(priority)];
        switch (affinityMask)
        {
        case FiberAffinityMask::kAll:
            globalQueueSet.m_jobQueues[festd::to_underlying(ThreadPoolType::kGeneric)].Enqueue(job);
            break;

        case FiberAffinityMask::kAllForeground:
            globalQueueSet.m_jobQueues[festd::to_underlying(ThreadPoolType::kForeground)].Enqueue(job);
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
