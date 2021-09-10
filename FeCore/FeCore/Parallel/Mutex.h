#pragma once
#include <FeCore/Parallel/Interlocked.h>

namespace FE
{
    class Mutex final
    {
        inline static constexpr USize NativeMutexSpace = 64;
        std::aligned_storage_t<NativeMutexSpace> m_NativeMutex;

    public:
        Mutex(const Mutex&) = delete;
        Mutex& operator=(const Mutex&) = delete;

        Mutex() noexcept;
        ~Mutex();

        void Lock() noexcept;
        bool TryLock();
        void Unlock();
    };
} // namespace FE
