#pragma once
#include <FeCore/Jobs/IJobSystem.h>
#include <FeCore/Modules/ServiceLocator.h>

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
            {
                IJobSystem* jobSystem = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();
                std::pmr::memory_resource* allocator = jobSystem->GetWaitGroupAllocator();
                allocator->deallocate(this, sizeof(WaitGroup), alignof(WaitGroup));
            }

            return refCount;
        }

        static WaitGroup* Create(const int32_t counter = 1)
        {
            IJobSystem* jobSystem = Env::GetServiceProvider()->ResolveRequired<IJobSystem>();
            std::pmr::memory_resource* allocator = jobSystem->GetWaitGroupAllocator();
            auto* result = new (allocator->allocate(sizeof(WaitGroup), alignof(WaitGroup))) WaitGroup;
            if (counter)
                result->Add(counter);
            return result;
        }

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

        void Add(int32_t value);
        void Signal();
        bool IsSignaled() const;
        void Wait();

    private:
        std::atomic<uint32_t> m_refCount = 0;
        std::atomic<int32_t> m_counter = 0;
        std::atomic<uint64_t> m_lockAndQueue = 0;

        bool SignalSlowImpl();

        WaitGroup() = default;
    };


    inline void WaitGroup::Add(const int32_t value)
    {
        FE_Assert(value > 0, "Invalid value");
        m_counter.fetch_add(value);
    }


    inline bool WaitGroup::IsSignaled() const
    {
        return m_counter.load() == 0;
    }
} // namespace FE
