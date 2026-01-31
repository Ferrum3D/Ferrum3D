#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Threading/Mutex.h>

namespace FE::Threading
{
    struct ConditionVariable final
    {
        ConditionVariable() noexcept;
        ~ConditionVariable() = default;

        ConditionVariable(const ConditionVariable&) = delete;
        ConditionVariable& operator=(const ConditionVariable&) = delete;

        void NotifyOne() noexcept;
        void NotifyAll() noexcept;

        void Wait(std::unique_lock<Mutex>& lock);
        bool WaitFor(std::unique_lock<Mutex>& lock, uint32_t milliseconds);

        template<class TPredicate>
        void Wait(std::unique_lock<Mutex>& lock, TPredicate&& predicate)
        {
            while (!predicate())
                Wait(lock);
        }

        template<class TPredicate>
        bool WaitFor(std::unique_lock<Mutex>& lock, const uint32_t milliseconds, TPredicate&& predicate)
        {
            while (!predicate())
            {
                if (!WaitFor(lock, milliseconds))
                    return predicate();
            }
            return true;
        }

    private:
        uint64_t m_nativeConditionVariable[2] = {};
    };
} // namespace FE::Threading
