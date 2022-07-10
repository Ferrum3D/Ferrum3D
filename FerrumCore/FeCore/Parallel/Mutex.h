#pragma once
#include <FeCore/Parallel/Interlocked.h>

namespace FE
{
    //! \brief Null mutex, never blocks.
    class NullMutex final
    {
    public:
        NullMutex(const NullMutex&) = delete;
        NullMutex& operator=(const NullMutex&) = delete;

        NullMutex()  = default;
        ~NullMutex() = default;

        inline void Lock() noexcept {}

        [[nodiscard]] inline bool TryLock() const noexcept // NOLINT
        {
            return true;
        }

        inline void Unlock() noexcept {}
    };

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
