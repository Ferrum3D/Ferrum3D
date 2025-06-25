#include <FeCore/Base/Platform.h>
#include <FeCore/Jobs/JobSystem.h>
#include <FeCore/Jobs/WaitGroup.h>
#include <FeCore/Strings/Format.h>

namespace FE
{
    void JobSystem::AddReadyFiber(FiberWaitEntry* entry)
    {
        entry->m_orderHint = m_jobCounter.fetch_add(1, std::memory_order_relaxed);

        const FiberAffinityMask affinityMask = entry->m_affinityMask;
        const JobPriority priority = entry->m_priority;
        GlobalQueueSet& globalQueueSet = m_globalQueues[festd::to_underlying(priority)];
        switch (affinityMask)
        {
        case FiberAffinityMask::kAll:
            globalQueueSet.m_readyFiberQueues[festd::to_underlying(JobThreadPoolType::kGeneric)].Enqueue(entry);
            break;

        case FiberAffinityMask::kAllForeground:
            globalQueueSet.m_readyFiberQueues[festd::to_underlying(JobThreadPoolType::kForeground)].Enqueue(entry);
            break;

        case FiberAffinityMask::kAllBackground:
            globalQueueSet.m_readyFiberQueues[festd::to_underlying(JobThreadPoolType::kBackground)].Enqueue(entry);
            break;

        case FiberAffinityMask::kNone:
        case FiberAffinityMask::kMainThread:
        default:
            {
                FE_AssertDebug(Bit::PopCount(festd::to_underlying(affinityMask)) == 1, "Invalid affinity mask");
                const uint32_t threadIndex = Bit::CountTrailingZeros(festd::to_underlying(affinityMask));

                Worker& worker = m_workers[threadIndex];
                worker.m_readyFiberQueues[festd::to_underlying(priority)].Enqueue(entry);
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
            Job* job = nullptr;
            auto affinityMask = FiberAffinityMask::kNone;

            for (uint32_t attempt = 0; attempt < 8; ++attempt)
            {
                for (int32_t queueIndex = festd::to_underlying(JobPriority::kHigh); queueIndex >= 0; --queueIndex)
                {
                    GlobalQueueSet& globalQueueSet = m_globalQueues[queueIndex];

                    // Firstly, try to continue pending fiber.

                    FiberWaitEntry* fibers[3] = {};
                    ConcurrentQueue* queues[3] = {};

                    queues[0] = &worker.m_readyFiberQueues[queueIndex];
                    queues[1] = &globalQueueSet.m_readyFiberQueues[festd::to_underlying(JobThreadPoolType::kGeneric)];
                    queues[2] = &globalQueueSet.m_readyFiberQueues[festd::to_underlying(worker.m_threadPoolType)];

                    // Since we use single-consumer queues, and we might push a job back to the front of the queue,
                    // we need this lock here.

                    std::unique_lock lock{ globalQueueSet.m_consumerLocks[queueIndex] };

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
                        worker.m_priority = static_cast<JobPriority>(queueIndex);
                        waitEntry = fibers[fiberIndex];

                        switch (fiberIndex)
                        {
                        case 0:
                            affinityMask = static_cast<FiberAffinityMask>(1 << GetWorkerIndex());
                            break;
                        case 1:
                            affinityMask = FiberAffinityMask::kAll;
                            break;
                        case 2:
                            affinityMask = worker.m_threadPoolType == JobThreadPoolType::kForeground
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

                    Job* jobs[3] = {};

                    queues[0] = &worker.m_jobQueues[queueIndex];
                    queues[1] = &globalQueueSet.m_jobQueues[festd::to_underlying(JobThreadPoolType::kGeneric)];
                    queues[2] = &globalQueueSet.m_jobQueues[festd::to_underlying(worker.m_threadPoolType)];

                    for (uint32_t i = 0; i < 3; ++i)
                        jobs[i] = static_cast<Job*>(queues[i]->TryDequeue());

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
                        worker.m_priority = static_cast<JobPriority>(queueIndex);
                        job = jobs[jobIndex];

                        switch (jobIndex)
                        {
                        case 0:
                            affinityMask = static_cast<FiberAffinityMask>(1 << GetWorkerIndex());
                            break;
                        case 1:
                            affinityMask = FiberAffinityMask::kAll;
                            break;
                        case 2:
                            affinityMask = worker.m_threadPoolType == JobThreadPoolType::kForeground
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

            // TODO: sleep
            std::this_thread::yield();
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
        const Platform::CpuInfo cpuInfo = Platform::GetCpuInfo();
        const uint32_t threadCount = cpuInfo.m_physicalCores < 8 ? cpuInfo.m_logicalCores : cpuInfo.m_physicalCores;
        const uint32_t workerCount = Math::Clamp(threadCount, 4u, kMaxWorkerCount);
        // const uint32_t workerCount = 2;
        const uint32_t foregroundWorkerCount = Math::CeilDivide(workerCount, 2);
        const uint32_t backgroundWorkerCount = workerCount - foregroundWorkerCount;

        Worker& mainThread = m_workers[0];
        mainThread.m_threadId = Threading::GetCurrentThreadID();
        mainThread.m_name = "Main Thread";
        mainThread.m_threadPoolType = JobThreadPoolType::kForeground;

        for (uint32_t workerIndex = 1; workerIndex < kMaxWorkerCount; ++workerIndex)
        {
            const bool isForeground = workerIndex < foregroundWorkerCount;
            const bool isBackground = workerIndex >= 32 && workerIndex < 32 + backgroundWorkerCount;
            if (!isForeground && !isBackground)
                continue;

            const auto threadName = isForeground ? Fmt::FixedFormat("Foreground Worker {}", workerIndex)
                                                 : Fmt::FixedFormat("Background Worker {}", workerIndex - 32);
            const auto threadFunc = [](const uintptr_t workerIndex) {
                fe_assert_cast<JobSystem*>(Env::GetServiceProvider()->ResolveRequired<IJobSystem>())
                    ->ThreadProc(static_cast<uint32_t>(workerIndex));
            };

            Worker& worker = m_workers[workerIndex];
            worker.m_thread = Threading::CreateThread(threadName, threadFunc, workerIndex);
            worker.m_name = threadName;
            worker.m_threadPoolType = isForeground ? JobThreadPoolType::kForeground : JobThreadPoolType::kBackground;
        }

        m_foregroundWorkerCount = foregroundWorkerCount;
        m_backgroundWorkerCount = backgroundWorkerCount;
    }


    JobSystem::~JobSystem()
    {
        m_shouldExit.store(true, std::memory_order_release);
        for (Worker& worker : m_workers)
        {
            Threading::CloseThread(worker.m_thread);
        }
    }


    void JobSystem::Start()
    {
        const Threading::FiberHandle initialFiber = m_fiberPool.Rent(false);
        Worker& mainThread = m_workers[0];
        mainThread.m_currentFiber = initialFiber;
        FE_Assert(mainThread.m_threadId == Threading::GetCurrentThreadID());
        m_semaphore.Release(m_backgroundWorkerCount + m_foregroundWorkerCount - 1);
        m_fiberPool.Switch(initialFiber, reinterpret_cast<uintptr_t>(this), mainThread.m_name.c_str());
    }


    void JobSystem::Stop()
    {
        m_shouldExit.store(true, std::memory_order_release);
    }


    void JobSystem::Schedule(const JobScheduleInfo& info)
    {
        info.m_job->m_orderHint = m_jobCounter.fetch_add(1, std::memory_order_relaxed);

        const FiberAffinityMask affinityMask = info.m_affinityMask;
        const JobPriority priority = info.m_priority;
        GlobalQueueSet& globalQueueSet = m_globalQueues[festd::to_underlying(priority)];
        switch (affinityMask)
        {
        case FiberAffinityMask::kAll:
            globalQueueSet.m_jobQueues[festd::to_underlying(JobThreadPoolType::kGeneric)].Enqueue(info.m_job);
            break;

        case FiberAffinityMask::kAllForeground:
            globalQueueSet.m_jobQueues[festd::to_underlying(JobThreadPoolType::kForeground)].Enqueue(info.m_job);
            break;

        case FiberAffinityMask::kAllBackground:
            globalQueueSet.m_jobQueues[festd::to_underlying(JobThreadPoolType::kBackground)].Enqueue(info.m_job);
            break;

        case FiberAffinityMask::kNone:
        case FiberAffinityMask::kMainThread:
        default:
            {
                FE_AssertDebug(Bit::PopCount(festd::to_underlying(affinityMask)) == 1, "Invalid affinity mask");
                const uint32_t threadIndex = Bit::CountTrailingZeros(festd::to_underlying(affinityMask));

                Worker& worker = m_workers[threadIndex];
                worker.m_jobQueues[festd::to_underlying(priority)].Enqueue(info.m_job);
                break;
            }
        }
    }


    FiberAffinityMask JobSystem::GetAffinityMaskForCurrentThread() const
    {
        return static_cast<FiberAffinityMask>(UINT64_C(1) << GetWorkerIndex());
    }
} // namespace FE
