#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    //! \brief Null mutex, never blocks.
    class NullMutex final
    {
    public:
        NullMutex(const NullMutex&) = delete;
        NullMutex& operator=(const NullMutex&) = delete;

        NullMutex() = default;
        ~NullMutex() = default;

        inline void lock() noexcept {}

        [[nodiscard]] inline bool try_lock() const noexcept // NOLINT
        {
            return true;
        }

        inline void unlock() noexcept {}
    };


    class Mutex final
    {
        inline static constexpr USize NativeMutexSpace = 64;
        std::aligned_storage_t<NativeMutexSpace> m_NativeMutex;

    public:
        Mutex(const Mutex&) = delete;
        Mutex& operator=(const Mutex&) = delete;

        inline Mutex() noexcept
            : Mutex(500)
        {
        }

        Mutex(uint32_t spinCount) noexcept;
        ~Mutex();

        void lock() noexcept;
        bool try_lock();
        void unlock();
    };
} // namespace FE
