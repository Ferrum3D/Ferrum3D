#pragma once
#include <Core/Jobs/WaitGroup.h>
#include <Core/Threading/SpinLock.h>

namespace FE::Threading
{
    // Non-recursive mutex. Contended acquisition suspends the current job fiber.
    struct FiberMutex final
    {
        FiberMutex() = default;
        ~FiberMutex();
        FiberMutex(const FiberMutex&) = delete;
        FiberMutex& operator=(const FiberMutex&) = delete;
        void lock();
        bool try_lock();
        void unlock();
        [[nodiscard]] uint32_t GetWaiterCount();

    private:
        struct Waiter
        {
            Rc<WaitGroup> m_ready;
            Waiter* m_next = nullptr;
        };
        SpinLock m_lock;
        Waiter* m_first = nullptr;
        Waiter* m_last = nullptr;
        uint32_t m_waiters = 0;
        bool m_held = false;
    };
} // namespace FE::Threading
