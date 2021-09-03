#pragma once
#include <FeCore/Parallel/Mutex.h>
#include <FeCore/Parallel/SpinMutex.h>

namespace FE
{
    namespace Internal
    {
        // clang-format off
        struct LockTypeDefer{};     //!< \brief Do not acquire ownership of the mutex.
        struct LockTypeTryToLock{}; //!< \brief Try to acquire ownership of the mutex without blocking.
        struct LockTypeAdopt{};     //!< \brief Assume the calling thread already has ownership of the mutex.
        // clang-format on
    } // namespace Internal

    namespace LockType
    {
        inline constexpr Internal::LockTypeDefer Defer{};         //!< \brief Do not acquire ownership of the mutex.
        inline constexpr Internal::LockTypeTryToLock TryToLock{}; //!< \brief Try to acquire ownership of the mutex without blocking.
        inline constexpr Internal::LockTypeAdopt Adopt{};         //!< \brief Assume the calling thread already has ownership of the mutex.

    } // namespace LockType

    template<class TLock>
    class Locker final
    {
        TLock* m_Lock;

    public:
        Locker(const Locker&) = delete;
        Locker& operator=(const Locker&) = delete;

        FE_FINLINE explicit Locker(TLock& lk)
            : m_Lock(&lk)
        {
            m_Lock->Lock();
        }

        FE_FINLINE Locker(TLock& lk, Internal::LockTypeAdopt)
            : m_Lock(&lk)
        {
        }

        FE_FINLINE ~Locker()
        {
            m_Lock->Unlock();
        }
    };

    template<class TLock>
    class UniqueLocker final
    {
        TLock* m_Lock;
        bool m_IsOwner;

    public:
        UniqueLocker(const UniqueLocker&) = delete;
        UniqueLocker& operator=(const UniqueLocker&) = delete;

        FE_FINLINE UniqueLocker(UniqueLocker&& other) noexcept
            : m_Lock(other.m_Lock)
            , m_IsOwner(other.m_IsOwner)
        {
            other.m_Lock    = nullptr;
            other.m_IsOwner = false;
        }

        FE_FINLINE UniqueLocker& operator=(UniqueLocker&& other) noexcept
        {
            if (m_IsOwner)
            {
                m_Lock->Unlock();
            }

            m_Lock          = other.m_Lock;
            m_IsOwner       = other.m_IsOwner;
            other.m_Lock    = nullptr;
            other.m_IsOwner = false;
        }

        FE_FINLINE UniqueLocker()
            : m_Lock(nullptr)
            , m_IsOwner(false)
        {
        }

        FE_FINLINE ~UniqueLocker()
        {
            if (m_IsOwner)
            {
                m_Lock->Unlock();
            }
        }

        FE_FINLINE explicit UniqueLocker(TLock& lk)
            : m_Lock(&lk)
            , m_IsOwner(false)
        {
            m_Lock->Lock();
            m_IsOwner = true;
        }

        FE_FINLINE UniqueLocker(TLock& lk, Internal::LockTypeDefer)
            : m_Lock(&lk)
            , m_IsOwner(false)
        {
        }

        FE_FINLINE UniqueLocker(TLock& lk, Internal::LockTypeTryToLock)
            : m_Lock(&lk)
        {
            m_IsOwner = m_Lock->TryLock();
        }

        FE_FINLINE UniqueLocker(TLock& lk, Internal::LockTypeAdopt)
            : m_Lock(&lk)
            , m_IsOwner(true)
        {
        }

        FE_FINLINE void Lock()
        {
            FE_CORE_ASSERT(m_Lock, "Lock was nullptr");
            m_Lock->Lock();
            m_IsOwner = true;
        }

        FE_FINLINE bool TryLock()
        {
            FE_CORE_ASSERT(m_Lock, "Lock was nullptr");
            m_IsOwner = m_Lock->TryLock();
            return m_IsOwner;
        }

        FE_FINLINE void Unlock()
        {
            FE_CORE_ASSERT(m_Lock, "Lock was nullptr");
            FE_CORE_ASSERT(m_IsOwner, "Only the owner can unlock a lock");
            m_Lock->Unlock();
            m_IsOwner = false;
        }

        FE_FINLINE TLock* Detach()
        {
            auto* t   = m_Lock;
            m_Lock    = nullptr;
            m_IsOwner = false;
            return m_Lock;
        }

        [[nodiscard]] FE_FINLINE bool IsOwner() const noexcept
        {
            return m_IsOwner;
        }
    };
} // namespace FE
