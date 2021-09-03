#pragma once
#include <FeCore/Parallel/Interlocked.h>
#include <thread>

namespace FE
{
    namespace Internal
    {
        inline constexpr Int32 MaxSpinLockPauseCount = 32;

        struct SpinLockCounter final
        {
            Int32 Count;

            inline SpinLockCounter() noexcept
                : Count(1)
            {
            }

            FE_FINLINE void Reset() noexcept
            {
                Count = 1;
            }

            FE_FINLINE void Wait() noexcept
            {
                if (Count <= MaxSpinLockPauseCount)
                {
                    for (Int32 i = 0; i < Count; ++i)
                    {
                        _mm_pause();
                    }

                    Count <<= 1;
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        };
    } // namespace Internal

    class SpinMutex final
    {
        AtomicInt32 m_Flag;

    public:
        SpinMutex(const SpinMutex&) = delete;
        SpinMutex& operator=(const SpinMutex&) = delete;

        FE_FINLINE explicit SpinMutex(bool locked = false) // NOLINT
        {
            Interlocked::Exchange(m_Flag, locked);
        }

        FE_FINLINE bool TryLock() noexcept
        {
            return Interlocked::CompareExchange(m_Flag, 1, 0);
        }

        FE_FINLINE void Lock() noexcept
        {
            if (!TryLock())
            {
                Internal::SpinLockCounter counter;

                while (!TryLock())
                {
                    counter.Wait();
                }
            }
        }

        FE_FINLINE void Unlock() noexcept
        {
            Interlocked::Exchange(m_Flag, 0);
        }
    };
} // namespace FE
