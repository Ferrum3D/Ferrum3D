#pragma once
#include <Core/Jobs/Base.h>
#include <Core/Env/Environment.h>

namespace FE
{
    struct WaitGroupWaitEntry;

    struct WaitGroup final
    {
        WaitGroup() = default;

        [[nodiscard]] uint32_t GetRefCount() const
        {
            return m_refCount.load(std::memory_order_relaxed);
        }

        uint32_t AddRef()
        {
            return ++m_refCount;
        }

        uint32_t Release()
        {
            const uint32_t refCount = --m_refCount;
            if (refCount == 0)
                DestroyImpl();

            return refCount;
        }

        static WaitGroup* Create(uint32_t counter = 1);

        static void WaitAll(festd::span<WaitGroup* const> waitGroups);
        static void WaitAll(festd::span<const Rc<WaitGroup>> waitGroups);

        static void WaitAll(const std::initializer_list<WaitGroup*> waitGroups)
        {
            WaitAll(festd::span(waitGroups));
        }

        static void WaitAll(const std::initializer_list<const Rc<WaitGroup>> waitGroups)
        {
            WaitAll(festd::span(waitGroups));
        }

        void Add(int32_t value);
        void SignalAll();
        void Signal();
        bool IsSignaled() const;
        void Wait();

    private:
        friend struct Jobs::JobNode;

        std::atomic<uint32_t> m_refCount = 0;
        std::atomic<int32_t> m_counter = 0;
        std::atomic<uint64_t> m_lockAndQueue = 0;

        void SignalImpl();
        bool AddWaitEntry(WaitGroupWaitEntry* entry);
        void AddJobPrerequisite(Jobs::JobNode* job);
        bool SignalSlowImpl();
        static void SignalJobWaitEntry(WaitGroupWaitEntry* baseEntry);
        static void SignalFiberWaitEntry(WaitGroupWaitEntry* baseEntry);
        void DestroyImpl();
    };


    inline void WaitGroup::Add(const int32_t value)
    {
        FE_AssertDebug(value > 0, "Invalid value");
        m_counter.fetch_add(value);
    }


    FE_FORCE_INLINE void WaitGroup::SignalAll()
    {
        const int32_t prevValue = m_counter.exchange(0);
        FE_Assert(prevValue > 0);
        SignalImpl();
    }


    FE_FORCE_INLINE void WaitGroup::Signal()
    {
        const int32_t prevValue = m_counter.fetch_sub(1);
        if (prevValue > 1)
            return;

        FE_Assert(prevValue == 1);
        SignalImpl();
    }


    inline bool WaitGroup::IsSignaled() const
    {
        return m_counter.load() == 0;
    }
} // namespace FE
