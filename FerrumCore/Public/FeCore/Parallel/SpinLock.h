#pragma once
#include <atomic>

namespace FE
{
    class SpinLock final
    {
        std::atomic<bool> m_Locked = false;

    public:
        inline void lock() noexcept
        {
            if (!m_Locked.exchange(true, std::memory_order_acquire))
                return;

            _mm_pause();

            while (true)
            {
                if (!m_Locked.exchange(true, std::memory_order_acquire))
                    return;

                uint32_t spinCount = 1;
                while (m_Locked.load(std::memory_order_relaxed))
                {
                    for (uint32_t i = 0; i < spinCount; ++i)
                        _mm_pause();

                    spinCount = std::min(spinCount << 1, 32u);
                }
            }
        }

        inline bool try_lock() noexcept
        {
            return !m_Locked.load(std::memory_order_relaxed) && !m_Locked.exchange(true);
        }

        inline void unlock() noexcept
        {
            m_Locked.store(false, std::memory_order_release);
        }
    };
} // namespace FE
