#pragma once
#include <atomic>

namespace FE::Threading
{
    struct SpinLock final
    {
        void lock() noexcept
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

                    spinCount = std::min(spinCount << 1, 32u);
                }
            }
        }

        bool try_lock() noexcept
        {
            return !m_locked.load(std::memory_order_relaxed) && !m_locked.exchange(true);
        }

        void unlock() noexcept
        {
            m_locked.store(false, std::memory_order_release);
        }

    private:
        std::atomic<bool> m_locked = false;
    };
} // namespace FE::Threading
