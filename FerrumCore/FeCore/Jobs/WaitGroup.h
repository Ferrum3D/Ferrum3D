#pragma once
#include <FeCore/Jobs/IJobSystem.h>
#include <FeCore/Modules/ServiceLocator.h>

namespace FE
{
    class WaitGroup final
    {
        std::atomic<uint32_t> m_RefCount;
        std::atomic<int32_t> m_Counter;
        std::atomic<uint64_t> m_LockAndQueue;

        bool SignalSlowImpl();

        WaitGroup() = default;

    public:
        [[nodiscard]] inline uint32_t GetRefCount() const
        {
            return m_RefCount.load(std::memory_order_relaxed);
        }

        inline uint32_t AddRef()
        {
            return ++m_RefCount;
        }

        inline uint32_t Release()
        {
            const uint32_t refCount = --m_RefCount;
            if (refCount == 0)
                Env::GetServiceProvider()->ResolveRequired<IJobSystem>()->FreeSmallBlock(this, sizeof(WaitGroup));

            return refCount;
        }

        inline static WaitGroup* Create()
        {
            void* ptr = Env::GetServiceProvider()->ResolveRequired<IJobSystem>()->AllocateSmallBlock(sizeof(WaitGroup));
            return new (ptr) WaitGroup;
        }

        void Add(int32_t value);
        void Signal();
        bool IsSignaled() const;
        void Wait();
    };


    inline void WaitGroup::Add(int32_t value)
    {
        FE_CORE_ASSERT(value > 0, "Invalid value");
        m_Counter.fetch_add(value);
    }


    inline bool WaitGroup::IsSignaled() const
    {
        return m_Counter.load() == 0;
    }
} // namespace FE
