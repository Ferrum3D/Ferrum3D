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
        m_Semaphore.Acquire();
        m_Workers[workerIndex].ThreadID = GetCurrentThreadID();

        const FiberHandle initialFiber = m_FiberPool.Rent(false);
        m_Workers[workerIndex].CurrentFiber = initialFiber;
        m_Workers[workerIndex].PrevFiber.Reset();
        m_FiberPool.Switch(initialFiber, reinterpret_cast<uintptr_t>(this));
    }


    FE_FORCE_INLINE void JobSystem::FiberProc(Context::TransferParams transferParams)
    {
        const uint32_t initialWorkerIndex = GetWorkerIndex();
        if (!m_Workers[initialWorkerIndex].ExitContext)
            m_Workers[initialWorkerIndex].ExitContext = transferParams.ContextHandle;

        CleanUpAfterSwitch(transferParams);
        while (!m_ShouldExit.load(std::memory_order_acquire))
        {
            Worker& worker = m_Workers[GetWorkerIndex()];

            FiberWaitEntry* pWaitEntry = nullptr;
            Job* pJob = nullptr;
            {
                ZoneNamed(TryDequeue, true);
                for (uint32_t attempt = 0; attempt < 8; ++attempt)
                {
                    for (int32_t queueIndex = enum_cast(JobPriority::High); queueIndex >= 0; --queueIndex)
                    {
                        JobQueue& queue = m_JobQueues[queueIndex];
                        std::lock_guard lk{ queue.Lock };
                        if (!queue.ReadyFibersQueue.empty())
                        {
                            FiberWaitEntry* pEntry = &queue.ReadyFibersQueue.front();
                            if (pEntry->SwitchCompleted)
                            {
                                queue.ReadyFibersQueue.pop_front();
                                pWaitEntry = pEntry;
                                break;
                            }
                        }

                        if (!queue.Queue.empty())
                        {
                            worker.Priority = static_cast<JobPriority>(queueIndex);
                            pJob = &queue.Queue.front();
                            queue.Queue.pop_front();
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

            if (pWaitEntry)
            {
                worker.PrevFiber = worker.CurrentFiber;
                worker.CurrentFiber = pWaitEntry->Fiber;
                transferParams = m_FiberPool.Switch(worker.CurrentFiber, reinterpret_cast<uintptr_t>(this));
                CleanUpAfterSwitch(transferParams);
                continue;
            }
            if (pJob)
            {
                Rc completionWaitGroup = pJob->m_pCompletionWaitGroup;
                pJob->Execute();
                if (completionWaitGroup)
                    completionWaitGroup->Signal();

                continue;
            }

            // TODO: sleep
            std::this_thread::yield();
        }

        TracyFiberLeave;
        Context::Switch(m_Workers[GetWorkerIndex()].ExitContext, 0);
    }


    void JobSystem::FiberProcImpl(Context::TransferParams transferParams)
    {
        const uintptr_t jobSystemAddress = transferParams.UserData & ((UINT64_C(1) << 48) - 1);
        reinterpret_cast<JobSystem*>(jobSystemAddress)->FiberProc(transferParams);
    }


    JobSystem::JobSystem()
        : m_FiberPool(&FiberProcImpl)
    {
        const int32_t workerCount = std::thread::hardware_concurrency() - 1;

        m_Workers.reserve(workerCount + 1);
        for (int32_t workerIndex = 0; workerIndex < workerCount; ++workerIndex)
        {
            const String threadName = Fmt::Format("Worker {}", workerIndex);
            const auto threadFunc = [](uintptr_t workerIndex) {
                fe_assert_cast<JobSystem*>(Env::GetServiceProvider()->ResolveRequired<IJobSystem>())
                    ->ThreadProc(static_cast<uint32_t>(workerIndex));
            };

            Worker& worker = m_Workers.push_back();
            worker.Thread = CreateThread(threadName, threadFunc, workerIndex);
        }
    }


    JobSystem::~JobSystem()
    {
        m_ShouldExit.store(true, std::memory_order_release);
        for (Worker& worker : m_Workers)
        {
            CloseThread(worker.Thread);
        }
    }


    void JobSystem::Start()
    {
        const FiberHandle initialFiber = m_FiberPool.Rent(false);
        Worker& mainThread = m_Workers.push_back();
        mainThread.ThreadID = GetCurrentThreadID();
        mainThread.CurrentFiber = initialFiber;
        mainThread.PrevFiber.Reset();
        m_Semaphore.Release(m_Workers.size() - 1);
        m_FiberPool.Switch(initialFiber, reinterpret_cast<uintptr_t>(this));
    }


    void JobSystem::Stop()
    {
        m_ShouldExit.store(true, std::memory_order_release);
    }


    void JobSystem::AddJob(Job* pJob, JobPriority priority)
    {
        JobQueue& queue = m_JobQueues[enum_cast(priority)];

        std::lock_guard lk{ queue.Lock };
        queue.Queue.push_back(*pJob);
    }
} // namespace FE
