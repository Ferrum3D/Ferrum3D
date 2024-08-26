#include <FeCore/Jobs/JobSystem.h>
#include <FeCore/Jobs/WaitGroup.h>

namespace FE
{
    FE_FORCE_INLINE bool WaitGroup::SignalSlowImpl()
    {
        uint64_t lockAndQueue = m_LockAndQueue.load(std::memory_order_acquire);
        if (lockAndQueue == 0)
        {
            if (m_LockAndQueue.compare_exchange_weak(lockAndQueue, 1, std::memory_order_acquire))
            {
                // The queue is empty
                return true;
            }

            return false;
        }

        if (lockAndQueue & 1)
            return false;

        return m_LockAndQueue.compare_exchange_weak(lockAndQueue, lockAndQueue | 1, std::memory_order_acquire);
    }


    void WaitGroup::Signal()
    {
        const int32_t prevValue = m_Counter.fetch_sub(1);
        if (prevValue > 1)
            return;

        uint32_t spinCount = 1;
        while (true)
        {
            if (SignalSlowImpl())
                break;

            for (uint32_t spin = 0; spin < spinCount; ++spin)
                _mm_pause();

            spinCount = std::min(spinCount << 1, 32u);
        }

        const uint64_t lockAndQueue = m_LockAndQueue.load(std::memory_order_relaxed);
        if (lockAndQueue == 1)
        {
            m_LockAndQueue.store(0, std::memory_order_release);
            return;
        }

        IJobSystem* pJobSystemInterface = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();
        JobSystem* pJobSystem = fe_assert_cast<JobSystem*>(pJobSystemInterface);

        FiberWaitEntry* pEntry = reinterpret_cast<FiberWaitEntry*>(lockAndQueue & ~1);
        while (pEntry)
        {
            FiberWaitEntry* pNext = static_cast<FiberWaitEntry*>(pEntry->mpNext);
            pJobSystem->AddReadyFiber(pEntry);
            pEntry = pNext;
        }

        m_LockAndQueue.store(0, std::memory_order_release);
    }


    void WaitGroup::Wait()
    {
        uint32_t spinCount = 1;
        while (true)
        {
            if (m_Counter.load(std::memory_order_relaxed) == 0)
                return;

            uint64_t lockAndQueue = m_LockAndQueue.load(std::memory_order_acquire);
            if ((lockAndQueue & 1)
                || !m_LockAndQueue.compare_exchange_weak(lockAndQueue, lockAndQueue | 1, std::memory_order_acquire))
            {
                for (uint32_t spin = 0; spin < spinCount; ++spin)
                    _mm_pause();

                spinCount = std::min(spinCount << 1, 32u);
                continue;
            }

            break;
        }

        const uint64_t lockAndQueue = m_LockAndQueue.load(std::memory_order_relaxed);

        if (IsSignaled())
        {
            m_LockAndQueue.store(lockAndQueue & ~1, std::memory_order_release);
            return;
        }

        FiberWaitEntry* pQueueHead = reinterpret_cast<FiberWaitEntry*>(lockAndQueue & ~1);
        IJobSystem* pJobSystemInterface = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();
        JobSystem* pJobSystem = fe_assert_cast<JobSystem*>(pJobSystemInterface);

        const uint32_t workerIndex = pJobSystem->GetWorkerIndex();
        JobSystem::Worker& worker = pJobSystem->m_Workers[workerIndex];

        FiberWaitEntry waitEntry;
        waitEntry.mpPrev = nullptr;
        waitEntry.mpNext = nullptr;
        waitEntry.Priority = worker.Priority;
        waitEntry.Fiber = worker.CurrentFiber;
        if (pQueueHead)
        {
            // Queue tail is stored in mpPrev to reduce the sizeof(FiberWaitEntry)
            pQueueHead->mpPrev->mpNext = &waitEntry;
            pQueueHead->mpPrev = &waitEntry;
            m_LockAndQueue.store(lockAndQueue & ~1, std::memory_order_release);
        }
        else
        {
            waitEntry.mpPrev = &waitEntry;
            m_LockAndQueue.store(reinterpret_cast<uint64_t>(&waitEntry), std::memory_order_release);
        }

        pJobSystem->SwitchFromWaitingFiber(workerIndex, waitEntry);
    }
} // namespace FE
