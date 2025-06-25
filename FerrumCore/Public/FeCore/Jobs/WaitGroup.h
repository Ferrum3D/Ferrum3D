#pragma once
#include <FeCore/Jobs/Base.h>
#include <FeCore/Modules/Environment.h>

namespace FE
{
    struct WaitGroup final
    {
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

        static void WaitAll(const festd::span<WaitGroup* const> waitGroups)
        {
            for (WaitGroup* waitGroup : waitGroups)
                waitGroup->Wait();
        }

        static void WaitAll(const festd::span<const Rc<WaitGroup>> waitGroups)
        {
            for (const Rc<WaitGroup>& waitGroup : waitGroups)
                waitGroup->Wait();
        }

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
        std::atomic<uint32_t> m_refCount = 0;
        std::atomic<int32_t> m_counter = 0;
        std::atomic<uint64_t> m_lockAndQueue = 0;

        void SignalImpl();
        bool SignalSlowImpl();
        void DestroyImpl();

        WaitGroup() = default;
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
