#include <FeCore/Jobs/JobSystem.h>
#include <FeCore/Jobs/WaitGroup.h>
#include <FeCore/Memory/PoolAllocator.h>

namespace FE
{
    namespace
    {
        Memory::SpinLockedPoolAllocator GWaitGroupAllocator{ "WaitGroupAllocator", sizeof(WaitGroup) };
    } // namespace


    void WaitGroup::SignalImpl()
    {
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
        auto* jobSystem = fe_assert_cast<JobSystem*>(jobSystemInterface);

        auto entry = reinterpret_cast<FiberWaitEntry*>(lockAndQueue & ~1);
        while (entry)
        {
            auto* next = static_cast<FiberWaitEntry*>(entry->m_next);
            jobSystem->AddReadyFiber(entry);
            entry = next;
        }

        m_lockAndQueue.store(0, std::memory_order_release);
    }


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


    void WaitGroup::DestroyImpl()
    {
        GWaitGroupAllocator.deallocate(this, sizeof(WaitGroup), alignof(WaitGroup));
    }


    WaitGroup* WaitGroup::Create(const uint32_t counter)
    {
        FE_AssertDebug(counter <= Constants::kMaxI32);

        auto* group = new (GWaitGroupAllocator.allocate(sizeof(WaitGroup), alignof(WaitGroup))) WaitGroup;
        if (counter)
            group->Add(static_cast<int32_t>(counter));

        return group;
    }


    void WaitGroup::Wait()
    {
        FE_PROFILER_ZONE();

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

        auto* queueHead = reinterpret_cast<FiberWaitEntry*>(lockAndQueue & ~1);
        IJobSystem* jobSystemInterface = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();
        auto* jobSystem = fe_assert_cast<JobSystem*>(jobSystemInterface);

        const uint32_t workerIndex = jobSystem->GetWorkerIndex();
        FE_Assert(workerIndex != kInvalidIndex, "WaitGroup::Wait() can only be called from a fiber");

        const JobSystem::Worker& worker = jobSystem->m_workers[workerIndex];

        FiberWaitEntry waitEntry;
        waitEntry.m_next = nullptr;
        waitEntry.m_priority = worker.m_priority;
        waitEntry.m_affinityMask = worker.m_affinityMask;
        waitEntry.m_fiber = worker.m_currentFiber;
        if (queueHead)
        {
            queueHead->m_queueTail->m_next = &waitEntry;
            queueHead->m_queueTail = &waitEntry;
            m_lockAndQueue.store(lockAndQueue & ~1, std::memory_order_release);
        }
        else
        {
            waitEntry.m_queueTail = &waitEntry;
            m_lockAndQueue.store(reinterpret_cast<uint64_t>(&waitEntry), std::memory_order_release);
        }

        jobSystem->SwitchFromWaitingFiber(workerIndex, waitEntry);
    }
} // namespace FE
