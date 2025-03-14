#pragma once
#include <FeCore/Base/BaseMath.h>
#include <atomic>

namespace FE::Threading
{
    struct SpinLock final
    {
        void lock() noexcept;
        bool try_lock() noexcept;
        void unlock() noexcept;

    private:
        std::atomic<bool> m_locked = false;
    };


    FE_FORCE_INLINE void SpinLock::lock() noexcept
    {
        if (!m_locked.exchange(true, std::memory_order_acquire))
            return;

        _mm_pause();

        while (true)
        {
            if (!m_locked.exchange(true, std::memory_order_acquire))
                return;

            uint32_t spinCount = 1;
            while (m_locked.load(std::memory_order_relaxed))
            {
                for (uint32_t i = 0; i < spinCount; ++i)
                    _mm_pause();

                spinCount = Math::Min(spinCount << 1, 32u);
            }
        }
    }


    FE_FORCE_INLINE bool SpinLock::try_lock() noexcept
    {
        return !m_locked.load(std::memory_order_relaxed) && !m_locked.exchange(true);
    }


    FE_FORCE_INLINE void SpinLock::unlock() noexcept
    {
        m_locked.store(false, std::memory_order_release);
    }
} // namespace FE::Threading
