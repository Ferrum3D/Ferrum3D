#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    //! @brief Null mutex, never blocks.
    class NullMutex final
    {
    public:
        NullMutex(const NullMutex&) = delete;
        NullMutex& operator=(const NullMutex&) = delete;

        NullMutex() = default;
        ~NullMutex() = default;

        void lock() noexcept {}

        [[nodiscard]] inline bool try_lock() const noexcept // NOLINT
        {
            return true;
        }

        void unlock() noexcept {}
    };


    struct Mutex final
    {
        Mutex(const Mutex&) = delete;
        Mutex& operator=(const Mutex&) = delete;

        Mutex() noexcept
            : Mutex(500)
        {
        }

        Mutex(uint32_t spinCount) noexcept;
        ~Mutex();

        void lock() noexcept;
        bool try_lock();
        void unlock();

    private:
        static constexpr size_t kNativeMutexSpace = 64;
        std::aligned_storage_t<kNativeMutexSpace> m_nativeMutex;
    };
} // namespace FE
