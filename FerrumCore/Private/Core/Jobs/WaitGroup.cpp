#include <Core/Jobs/JobSystem.h>
#include <Core/Jobs/WaitGroup.h>
#include <Core/Memory/PoolAllocator.h>
#include <festd/vector.h>

namespace FE
{
    struct WaitGroupWaitEntry
    {
        WaitGroupWaitEntry* m_next = nullptr;
        void (*m_signal)(WaitGroupWaitEntry* baseEntry) = nullptr;
    };


    namespace
    {
        enum class FiberWaitStateValue : uint32_t
        {
            kRegistering,
            kWaiting,
            kCompleted,
        };


        struct JobWaitEntry final : public WaitGroupWaitEntry
        {
            Jobs::JobNode* m_job = nullptr;
        };


        struct FiberWaitState final
        {
            std::atomic<uint32_t> m_dependencyCounter = 1;
            std::atomic<FiberWaitStateValue> m_state = FiberWaitStateValue::kRegistering;
            Jobs::FiberWaitEntry m_waitEntry;
        };


        struct FiberWaitGroupEntry final : public WaitGroupWaitEntry
        {
            FiberWaitState* m_state = nullptr;
        };


        Memory::SpinLockedPool<WaitGroup> GWaitGroupAllocator{ "WaitGroupAllocator" };
        Memory::SpinLockedPool<JobWaitEntry> GJobWaitEntryAllocator{ "JobWaitEntryAllocator" };
    } // namespace


    bool WaitGroup::AddWaitEntry(WaitGroupWaitEntry* entry)
    {
        FE_Assert((reinterpret_cast<uintptr_t>(entry) & 1) == 0, "Wait entry must be aligned");

        uint32_t spinCount = 1;
        while (true)
        {
            uint64_t lockAndQueue = m_lockAndQueue.load(std::memory_order_acquire);
            if ((lockAndQueue & 1)
                || !m_lockAndQueue.compare_exchange_weak(lockAndQueue, lockAndQueue | 1, std::memory_order_acquire))
            {
                for (uint32_t spin = 0; spin < spinCount; ++spin)
                    _mm_pause();

                spinCount = Math::Min(spinCount << 1, 32u);
                continue;
            }

            auto* queueHead = reinterpret_cast<WaitGroupWaitEntry*>(lockAndQueue & ~UINT64_C(1));
            if (m_counter.load(std::memory_order_relaxed) == 0)
            {
                m_lockAndQueue.store(reinterpret_cast<uint64_t>(queueHead), std::memory_order_release);
                return false;
            }

            entry->m_next = queueHead;
            m_lockAndQueue.store(reinterpret_cast<uint64_t>(entry), std::memory_order_release);
            return true;
        }
    }


    void WaitGroup::AddJobPrerequisite(Jobs::JobNode* job)
    {
        auto* entry = GJobWaitEntryAllocator.New();
        entry->m_signal = &SignalJobWaitEntry;
        entry->m_job = job;

        if (!AddWaitEntry(entry))
            SignalJobWaitEntry(entry);
    }


    void WaitGroup::SignalJobWaitEntry(WaitGroupWaitEntry* baseEntry)
    {
        auto* entry = static_cast<JobWaitEntry*>(baseEntry);
        Jobs::JobNode* job = entry->m_job;
        GJobWaitEntryAllocator.Delete(entry);

        if (!job->DependencySatisfied())
            return;

        FE_Assert(job->m_dispatchRequested.load(std::memory_order_acquire), "Unscheduled job reached zero dependencies");
        Jobs::JobSystem::Get().AddReadyJob(job);
    }


    void WaitGroup::SignalFiberWaitEntry(WaitGroupWaitEntry* baseEntry)
    {
        auto* entry = static_cast<FiberWaitGroupEntry*>(baseEntry);
        FiberWaitState* state = entry->m_state;
        const uint32_t previousValue = state->m_dependencyCounter.fetch_sub(1, std::memory_order_acq_rel);
        FE_Assert(previousValue > 0, "Fiber dependency counter underflow");
        if (previousValue > 1)
            return;

        const FiberWaitStateValue previousState =
            state->m_state.exchange(FiberWaitStateValue::kCompleted, std::memory_order_acq_rel);
        FE_Assert(previousState != FiberWaitStateValue::kCompleted, "Fiber wait completed twice");
        if (previousState == FiberWaitStateValue::kRegistering)
            return;

        while (!state->m_waitEntry.m_switchCompleted.load(std::memory_order_acquire))
            _mm_pause();

        Jobs::JobSystem::Get().AddReadyFiber(&state->m_waitEntry);
    }


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
        auto* entry = reinterpret_cast<WaitGroupWaitEntry*>(lockAndQueue & ~UINT64_C(1));
        m_lockAndQueue.store(0, std::memory_order_release);

        Rc keepAlive(this);
        while (entry)
        {
            WaitGroupWaitEntry* next = entry->m_next;
            entry->m_signal(entry);
            entry = next;
        }
    }


    FE_FORCE_INLINE bool WaitGroup::SignalSlowImpl()
    {
        uint64_t lockAndQueue = m_lockAndQueue.load(std::memory_order_acquire);
        if (lockAndQueue == 0)
        {
            if (m_lockAndQueue.compare_exchange_weak(lockAndQueue, 1, std::memory_order_acquire))
                return true;

            return false;
        }

        if (lockAndQueue & 1)
            return false;

        return m_lockAndQueue.compare_exchange_weak(lockAndQueue, lockAndQueue | 1, std::memory_order_acquire);
    }


    void WaitGroup::DestroyImpl()
    {
        FE_AssertDebug(m_lockAndQueue.load(std::memory_order_relaxed) == 0, "Destroying a wait group with pending waiters");
        GWaitGroupAllocator.Delete(this);
    }


    WaitGroup* WaitGroup::Create(const uint32_t counter)
    {
        FE_AssertDebug(counter <= Constants::kMaxI32);

        auto* group = GWaitGroupAllocator.New();
        if (counter)
            group->Add(static_cast<int32_t>(counter));

        return group;
    }


    void WaitGroup::WaitAll(const festd::span<WaitGroup* const> waitGroups)
    {
        FE_PROFILER_ZONE();

        if (waitGroups.empty())
            return;

        FiberWaitState state;
        festd::inline_vector<FiberWaitGroupEntry, 8> entries;
        entries.resize(waitGroups.size());

        for (uint32_t index = 0; index < waitGroups.size(); ++index)
        {
            WaitGroup* waitGroup = waitGroups[index];
            FE_Assert(waitGroup != nullptr, "Wait group cannot be null");

            state.m_dependencyCounter.fetch_add(1, std::memory_order_relaxed);
            FiberWaitGroupEntry& entry = entries[index];
            entry.m_signal = &SignalFiberWaitEntry;
            entry.m_state = &state;
            if (!waitGroup->AddWaitEntry(&entry))
                SignalFiberWaitEntry(&entry);
        }

        const uint32_t previousValue = state.m_dependencyCounter.fetch_sub(1, std::memory_order_acq_rel);
        FE_Assert(previousValue > 0, "Fiber dependency counter underflow");
        if (previousValue == 1)
            return;

        Jobs::JobSystem& jobSystem = Jobs::JobSystem::Get();
        const uint32_t workerIndex = jobSystem.GetWorkerIndex();
        FE_Assert(workerIndex != kInvalidIndex, "WaitGroup::WaitAll() can only wait from a fiber");

        const Jobs::JobSystem::Worker& worker = jobSystem.m_workers[workerIndex];
        state.m_waitEntry.m_priority = worker.m_priority;
        state.m_waitEntry.m_affinityMask = worker.m_affinityMask;
        state.m_waitEntry.m_fiber = worker.m_currentFiber;
        state.m_waitEntry.m_switchCompleted.store(false, std::memory_order_relaxed);

        auto expectedState = FiberWaitStateValue::kRegistering;
        if (!state.m_state.compare_exchange_strong(expectedState, FiberWaitStateValue::kWaiting, std::memory_order_acq_rel))
        {
            FE_Assert(expectedState == FiberWaitStateValue::kCompleted, "Invalid fiber wait state");
            return;
        }

        jobSystem.SwitchFromWaitingFiber(workerIndex, state.m_waitEntry);
    }


    void WaitGroup::WaitAll(const festd::span<const Rc<WaitGroup>> waitGroups)
    {
        festd::inline_vector<WaitGroup*, 8> rawWaitGroups;
        rawWaitGroups.reserve(waitGroups.size());
        for (const Rc<WaitGroup>& waitGroup : waitGroups)
            rawWaitGroups.push_back(waitGroup.Get());

        WaitAll(rawWaitGroups);
    }


    void WaitGroup::Wait()
    {
        WaitGroup* waitGroups[] = { this };
        WaitAll(waitGroups);
    }
} // namespace FE
