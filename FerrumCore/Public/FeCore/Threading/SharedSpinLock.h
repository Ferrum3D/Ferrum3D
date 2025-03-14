#pragma once
#include <FeCore/Base/BaseMath.h>
#include <atomic>

namespace FE::Threading
{
    struct SharedSpinLock final
    {
        void lock_shared() noexcept;
        bool try_lock_shared() noexcept;
        void unlock_shared() noexcept;

        void lock() noexcept;
        bool try_lock() noexcept;
        void unlock() noexcept;

    private:
        static constexpr uint32_t kExclusiveBit = 1u << 31;
        std::atomic<uint32_t> m_value = 0;
    };


    FE_FORCE_INLINE void SharedSpinLock::lock_shared() noexcept
    {
        // Register our reader.
        if (m_value.fetch_add(1, std::memory_order_acquire) & kExclusiveBit)
        {
            // If there is already a writer, spin.
            uint32_t spinCount = 1;
            while (m_value.load(std::memory_order_acquire) & kExclusiveBit)
            {
                for (uint32_t i = 0; i < spinCount; ++i)
                    _mm_pause();

                spinCount = Math::Min(spinCount << 1, 32u);
            }
        }
    }


    FE_FORCE_INLINE bool SharedSpinLock::try_lock_shared() noexcept
    {
        if ((m_value.fetch_add(1, std::memory_order_acquire) & kExclusiveBit) == 0)
            return true;

        m_value.fetch_sub(1, std::memory_order_release);
        return false;
    }


    FE_FORCE_INLINE void SharedSpinLock::unlock_shared() noexcept
    {
        m_value.fetch_sub(1, std::memory_order_release);
    }


    FE_FORCE_INLINE void SharedSpinLock::lock() noexcept
    {
        uint32_t spinCount = 1;
        while (true)
        {
            uint32_t expected = 0;
            if (m_value.compare_exchange_weak(expected, kExclusiveBit, std::memory_order_acquire))
                break;

            for (uint32_t i = 0; i < spinCount; ++i)
                _mm_pause();

            spinCount = Math::Min(spinCount << 1, 32u);
        }
    }


    FE_FORCE_INLINE bool SharedSpinLock::try_lock() noexcept
    {
        uint32_t expected = 0;
        return m_value.compare_exchange_strong(expected, kExclusiveBit, std::memory_order_acquire, std::memory_order_relaxed);
    }


    FE_FORCE_INLINE void SharedSpinLock::unlock() noexcept
    {
        m_value.fetch_sub(kExclusiveBit, std::memory_order_release);
    }
} // namespace FE::Threading
