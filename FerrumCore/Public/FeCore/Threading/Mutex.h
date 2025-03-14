#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Threading
{
    struct Mutex final
    {
        Mutex(const Mutex&) = delete;
        Mutex& operator=(const Mutex&) = delete;

        Mutex() noexcept
            : Mutex(500)
        {
        }

        explicit Mutex(uint32_t spinCount) noexcept;
        ~Mutex();

        void lock() noexcept;
        bool try_lock();
        void unlock();

    private:
        uint64_t m_nativeMutex[5];
    };
} // namespace FE::Threading
