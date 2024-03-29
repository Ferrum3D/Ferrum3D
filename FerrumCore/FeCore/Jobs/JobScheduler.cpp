#include <FeCore/Jobs/JobScheduler.h>

namespace FE
{
    thread_local SchedulerThreadInfo* JobScheduler::m_CurrentThreadInfo = nullptr;

    JobScheduler::JobScheduler(UInt32 workerCount)
        : m_WorkerCount(workerCount)
        , m_SleepingWorkerCount(0)
        , m_ShouldExit(0)
        , m_FrameIndex(0)
    {
        MonotonicAllocatorDesc desc{};
        desc.InitialBlockSize = AlignUp<alignof(SchedulerThreadInfo)>(sizeof(SchedulerThreadInfo)) * MaxThreadCount;
        desc.MaxBlockCount    = 1;
        m_ThreadInfoAllocator.Init(desc);
        desc.InitialBlockSize = 1024;
        desc.MaxBlockCount    = 32;
        m_OneFrameAllocators[0].Init(desc);
        m_OneFrameAllocators[1].Init(desc);

        auto& allocator = m_ThreadInfoAllocator;
        m_Threads.Reserve(m_WorkerCount);
        for (UInt32 i = 0; i < workerCount; ++i)
        {
            auto* thread = new (allocator.Allocate(sizeof(SchedulerThreadInfo), 16, FE_SRCPOS())) SchedulerThreadInfo;

            thread->WorkerID = i;
            thread->Thread   = std::thread(&JobScheduler::WorkerThreadProcess, this, i);
            thread->ThreadID = thread->Thread.get_id();
            m_Threads.Push(thread);
        }

        m_Semaphore.Release(GetWorkerCount());
    }

    UInt32 JobScheduler::GetWorkerCount() const
    {
        return m_WorkerCount;
    }

    UInt32 JobScheduler::GetWorkerID() const
    {
        return m_CurrentThreadInfo->WorkerID;
    }

    void JobScheduler::ScheduleJob(Job* job)
    {
        auto* thread = GetCurrentThread();

        if (job->Empty())
        {
            Execute(job);
            return;
        }
        if (thread->IsWorker())
        {
            thread->Queue.Enqueue(job);
            NotifyWorker();
            return;
        }
        m_GlobalQueue.Enqueue(job);
        NotifyWorker();
    }

    JobScheduler::~JobScheduler() noexcept
    {
        Interlocked::Exchange(m_ShouldExit, 1);
        for (UInt32 i = 0; i < m_WorkerCount; ++i)
        {
            m_Threads[i]->WaitSemaphore.Release();
            m_Threads[i]->Thread.join();
        }
        auto& allocator = m_ThreadInfoAllocator;
        for (auto t : m_Threads)
        {
            t->~SchedulerThreadInfo();
            allocator.Deallocate(t, FE_SRCPOS(), sizeof(SchedulerThreadInfo));
        }
    }

    void JobScheduler::WorkerThreadProcess(UInt32 id)
    {
        m_Semaphore.Acquire();
        m_CurrentThreadInfo = m_Threads[id];
        GlobalAllocator<HeapAllocator>::ThreadInit();
        ProcessJobs(nullptr);
    }

    void JobScheduler::Execute(Job* job)
    {
        JobExecutionContext context{};
        context.WorkerID = m_CurrentThreadInfo->WorkerID;
        job->ExecuteInternal(context);
        for (auto& t : m_Threads)
        {
            if (t->WaitJob == job)
            {
                t->WaitSemaphore.Release();
            }
        }
    }

    SchedulerThreadInfo* JobScheduler::GetCurrentThread()
    {
        if (!m_CurrentThreadInfo)
        {
            for (auto& thread : m_Threads)
            {
                if (thread->ThreadID == std::this_thread::get_id())
                {
                    m_CurrentThreadInfo = thread;
                }
            }
        }
        if (!m_CurrentThreadInfo)
        {
            auto& allocator  = m_ThreadInfoAllocator;
            auto* thread     = new (allocator.Allocate(sizeof(SchedulerThreadInfo), 16, FE_SRCPOS())) SchedulerThreadInfo;
            thread->ThreadID = std::this_thread::get_id();
            m_Threads.Push(thread);
            m_CurrentThreadInfo = thread;
        }
        return m_CurrentThreadInfo;
    }

    void JobScheduler::NotifyWorker()
    {
        while (Interlocked::Load(m_SleepingWorkerCount))
        {
            for (UInt32 i = 0; i < m_WorkerCount; ++i)
            {
                if (Interlocked::Exchange(m_Threads[i]->IsSleeping, 0) == 1)
                {
                    Interlocked::Decrement(m_SleepingWorkerCount);
                    m_Threads[i]->WaitSemaphore.Release();
                }
            }
        }
    }

    Job* JobScheduler::TryStealJob(UInt32& victimIndex)
    {
        const auto attempts = m_WorkerCount * 2;

        if (m_WorkerCount <= 1)
        {
            return nullptr;
        }

        for (UInt32 i = 0; i < attempts; ++i)
        {
            Job* job = m_Threads[victimIndex]->Queue.Steal();
            if (job)
            {
                return job;
            }

            victimIndex = (victimIndex + 1) % m_WorkerCount;
            if (victimIndex == m_CurrentThreadInfo->WorkerID)
            {
                victimIndex = (victimIndex + 1) % m_WorkerCount;
            }
        }

        return nullptr;
    }

    void* JobScheduler::OneFrameAllocate(USize size, USize alignment)
    {
        auto index = Interlocked::Load(m_FrameIndex);
        return m_OneFrameAllocators[index].Allocate(size, alignment, FE_SRCPOS());
    }

    void JobScheduler::OnFrameStart(const FrameEventArgs& args)
    {
        auto reset = Interlocked::Exchange(m_FrameIndex, static_cast<Int32>(args.FrameIndex & 1));
        m_OneFrameAllocators[reset].ResetAll();
    }

    void JobScheduler::SuspendUntilComplete(Job* job)
    {
        ProcessJobs(job);
    }

    void JobScheduler::ProcessJobs(Job* waitJob)
    {
        auto shouldExit = [this]() {
            return Interlocked::Load(m_ShouldExit) == 1;
        };
        auto waitJobReady = [waitJob]() {
            return waitJob && waitJob->GetExecutionState() == JobExecutionState::Complete;
        };

        UInt32 victimIndex = 0;
        if (m_CurrentThreadInfo->WorkerID == 0)
        {
            victimIndex = 1;
        }

        while (true)
        {
            if (waitJobReady())
            {
                return;
            }
            if (m_CurrentThreadInfo->IsWorker() && !waitJob)
            {
                if (shouldExit())
                {
                    return;
                }

                if (m_GlobalQueue.Empty())
                {
                    Interlocked::Increment(m_SleepingWorkerCount);
                    Interlocked::Exchange(m_CurrentThreadInfo->IsSleeping, 1);
                    m_CurrentThreadInfo->WaitJob = waitJob;
                    m_CurrentThreadInfo->WaitSemaphore.Acquire();

                    if (shouldExit())
                    {
                        return;
                    }
                }
            }

            if (waitJobReady())
            {
                return;
            }

            Job* job = m_GlobalQueue.Dequeue();
            if (job == nullptr)
            {
                job = m_CurrentThreadInfo->Queue.SelfSteal();
            }

            while (true)
            {
                while (job)
                {
                    Execute(job);
                    if (waitJobReady())
                    {
                        return;
                    }

                    job = m_CurrentThreadInfo->Queue.SelfSteal();
                    if (job)
                    {
                        NotifyWorker();
                    }
                }
                if (waitJobReady())
                {
                    return;
                }
                job = TryStealJob(victimIndex);
                if (!job)
                {
                    break;
                }
            }
        }
    }
} // namespace FE
