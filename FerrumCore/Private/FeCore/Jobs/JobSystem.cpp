#include <FeCore/Jobs/JobSystem.h>
#include <FeCore/Jobs/WaitGroup.h>
#include <FeCore/Strings/Format.h>

namespace FE
{
    void* JobSystem::AllocateSmallBlock(size_t byteSize)
    {
        return Memory::DefaultAllocate(byteSize);
    }


    void JobSystem::FreeSmallBlock(void* ptr, size_t byteSize)
    {
        (void)byteSize;
        Memory::DefaultFree(ptr);
    }


    FE_FORCE_INLINE void JobSystem::ThreadProc(uint32_t workerIndex)
    {
        m_semaphore.Acquire();
        m_workers[workerIndex].m_threadId = GetCurrentThreadID();

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
            Worker& worker = m_workers[GetWorkerIndex()];

            FiberWaitEntry* pWaitEntry = nullptr;
            Job* pJob = nullptr;
            {
                for (uint32_t attempt = 0; attempt < 8; ++attempt)
                {
                    for (int32_t queueIndex = festd::to_underlying(JobPriority::kHigh); queueIndex >= 0; --queueIndex)
                    {
                        JobQueue& queue = m_jobQueues[queueIndex];
                        std::lock_guard lk{ queue.m_lock };
                        if (!queue.m_readyFibersQueue.empty())
                        {
                            FiberWaitEntry* pEntry = &queue.m_readyFibersQueue.front();
                            if (pEntry->m_switchCompleted)
                            {
                                queue.m_readyFibersQueue.pop_front();
                                pWaitEntry = pEntry;
                                break;
                            }
                        }

                        if (!queue.m_queue.empty())
                        {
                            worker.m_priority = static_cast<JobPriority>(queueIndex);
                            pJob = &queue.m_queue.front();
                            queue.m_queue.pop_front();
                            break;
                        }
                    }

                    if (pJob || pWaitEntry)
                        break;

                    const uint32_t spinCount = std::min(1 << attempt, 32);
                    for (uint32_t spin = 0; spin < spinCount; ++spin)
                        _mm_pause();
                }
            }

            if (!m_initialJobPickedUp.exchange(true))
            {
                // The initial job must be picked up by the Main Thread.
                // After that we can safely let the other workers run.
                m_semaphore.Release(m_workers.size() - 1);
            }

            if (pWaitEntry)
            {
                worker.m_prevFiber = worker.m_currentFiber;
                worker.m_currentFiber = pWaitEntry->m_fiber;
                transferParams = m_fiberPool.Switch(worker.m_currentFiber, reinterpret_cast<uintptr_t>(this));
                CleanUpAfterSwitch(transferParams);
                continue;
            }
            if (pJob)
            {
                Rc completionWaitGroup = pJob->m_completionWaitGroup;
                pJob->Execute();
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


    void JobSystem::FiberProcImpl(Context::TransferParams transferParams)
    {
        const uintptr_t jobSystemAddress = transferParams.m_userData & ((UINT64_C(1) << 48) - 1);
        reinterpret_cast<JobSystem*>(jobSystemAddress)->FiberProc(transferParams);
    }


    JobSystem::JobSystem()
        : m_fiberPool(&FiberProcImpl)
    {
        const int32_t workerCount = std::thread::hardware_concurrency() - 1;

        m_workers.reserve(workerCount + 1);
        for (int32_t workerIndex = 0; workerIndex < workerCount; ++workerIndex)
        {
            const auto threadName = Fmt::FixedFormat("Worker {}", workerIndex);
            const auto threadFunc = [](uintptr_t workerIndex) {
                fe_assert_cast<JobSystem*>(Env::GetServiceProvider()->ResolveRequired<IJobSystem>())
                    ->ThreadProc(static_cast<uint32_t>(workerIndex));
            };

            Worker& worker = m_workers.push_back();
            worker.m_thread = CreateThread(threadName, threadFunc, workerIndex);
        }
    }


    JobSystem::~JobSystem()
    {
        m_shouldExit.store(true, std::memory_order_release);
        for (Worker& worker : m_workers)
        {
            CloseThread(worker.m_thread);
        }
    }


    void JobSystem::Start()
    {
        const Threading::FiberHandle initialFiber = m_fiberPool.Rent(false);
        Worker& mainThread = m_workers.push_back();
        mainThread.m_threadId = GetCurrentThreadID();
        mainThread.m_currentFiber = initialFiber;
        mainThread.m_prevFiber.Reset();
        m_fiberPool.Switch(initialFiber, reinterpret_cast<uintptr_t>(this));
    }


    void JobSystem::Stop()
    {
        m_shouldExit.store(true, std::memory_order_release);
    }


    void JobSystem::AddJob(Job* pJob, JobPriority priority)
    {
        JobQueue& queue = m_jobQueues[festd::to_underlying(priority)];

        std::lock_guard lk{ queue.m_lock };
        queue.m_queue.push_back(*pJob);
    }
} // namespace FE
