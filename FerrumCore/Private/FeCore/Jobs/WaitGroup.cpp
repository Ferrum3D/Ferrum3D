#include <FeCore/Jobs/JobSystem.h>
#include <FeCore/Jobs/WaitGroup.h>

namespace FE
{
    FE_FORCE_INLINE bool WaitGroup::SignalSlowImpl()
    {
        uint64_t lockAndQueue = m_lockAndQueue.load(std::memory_order_acquire);
        if (lockAndQueue == 0)
        {
            if (m_lockAndQueue.compare_exchange_weak(lockAndQueue, 1, std::memory_order_acquire))
            {
                // The queue is empty
                return true;
            }

            return false;
        }

        if (lockAndQueue & 1)
            return false;

        return m_lockAndQueue.compare_exchange_weak(lockAndQueue, lockAndQueue | 1, std::memory_order_acquire);
    }


    void WaitGroup::Signal()
    {
        const int32_t prevValue = m_counter.fetch_sub(1);
        if (prevValue > 1)
            return;

        uint32_t spinCount = 1;
        while (true)
        {
            if (SignalSlowImpl())
                break;

            for (uint32_t spin = 0; spin < spinCount; ++spin)
                _mm_pause();

            spinCount = Math::Min(spinCount << 1, 32u);
        }

        const uint64_t lockAndQueue = m_lockAndQueue.load(std::memory_order_relaxed);
        if (lockAndQueue == 1)
        {
            m_lockAndQueue.store(0, std::memory_order_release);
            return;
        }

        IJobSystem* jobSystemInterface = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();
        JobSystem* jobSystem = fe_assert_cast<JobSystem*>(jobSystemInterface);

        FiberWaitEntry* entry = reinterpret_cast<FiberWaitEntry*>(lockAndQueue & ~1);
        while (entry)
        {
            FiberWaitEntry* pNext = static_cast<FiberWaitEntry*>(entry->mpNext);
            jobSystem->AddReadyFiber(entry);
            entry = pNext;
        }

        m_lockAndQueue.store(0, std::memory_order_release);
    }


    void WaitGroup::Wait()
    {
        uint32_t spinCount = 1;
        while (true)
        {
            if (m_counter.load(std::memory_order_relaxed) == 0)
                return;

            uint64_t lockAndQueue = m_lockAndQueue.load(std::memory_order_acquire);
            if ((lockAndQueue & 1)
                || !m_lockAndQueue.compare_exchange_weak(lockAndQueue, lockAndQueue | 1, std::memory_order_acquire))
            {
                for (uint32_t spin = 0; spin < spinCount; ++spin)
                    _mm_pause();

                spinCount = Math::Min(spinCount << 1, 32u);
                continue;
            }

            break;
        }

        const uint64_t lockAndQueue = m_lockAndQueue.load(std::memory_order_relaxed);

        if (IsSignaled())
        {
            m_lockAndQueue.store(lockAndQueue & ~1, std::memory_order_release);
            return;
        }

        FiberWaitEntry* queueHead = reinterpret_cast<FiberWaitEntry*>(lockAndQueue & ~1);
        IJobSystem* jobSystemInterface = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();
        JobSystem* jobSystem = fe_assert_cast<JobSystem*>(jobSystemInterface);

        const uint32_t workerIndex = jobSystem->GetWorkerIndex();
        JobSystem::Worker& worker = jobSystem->m_workers[workerIndex];

        FiberWaitEntry waitEntry;
        waitEntry.mpPrev = nullptr;
        waitEntry.mpNext = nullptr;
        waitEntry.m_priority = worker.m_priority;
        waitEntry.m_fiber = worker.m_currentFiber;
        if (queueHead)
        {
            // Queue tail is stored in mpPrev to reduce the sizeof(FiberWaitEntry)
            queueHead->mpPrev->mpNext = &waitEntry;
            queueHead->mpPrev = &waitEntry;
            m_lockAndQueue.store(lockAndQueue & ~1, std::memory_order_release);
        }
        else
        {
            waitEntry.mpPrev = &waitEntry;
            m_lockAndQueue.store(reinterpret_cast<uint64_t>(&waitEntry), std::memory_order_release);
        }

        jobSystem->SwitchFromWaitingFiber(workerIndex, waitEntry);
    }
} // namespace FE
