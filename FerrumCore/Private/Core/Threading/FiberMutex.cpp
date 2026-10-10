#include <Core/Base/Base.h>
#include <Core/Jobs/JobSystem.h>
#include <Core/Threading/FiberMutex.h>

namespace FE::Threading
{
    struct FiberMutex::Waiter final
    {
        Jobs::FiberWaitEntry m_entry;
        Waiter* m_next = nullptr;
    };


    FiberMutex::~FiberMutex()
    {
        FE_Assert(!m_held && !m_first);
    }


    bool FiberMutex::try_lock()
    {
        std::lock_guard guard(m_lock);
        if (m_held)
            return false;

        m_held = true;
        return true;
    }


    void FiberMutex::lock()
    {
        Waiter waiter;
        {
            std::lock_guard guard(m_lock);
            if (!m_held)
            {
                m_held = true;
                return;
            }

            if (m_last)
                m_last->m_next = &waiter;
            else
                m_first = &waiter;

            m_last = &waiter;
            ++m_waiters;
        }

        Jobs::JobSystem::Get().WaitForSignal(waiter.m_entry);
    }


    void FiberMutex::unlock()
    {
        Waiter* ready;
        {
            std::lock_guard guard(m_lock);
            FE_Assert(m_held);
            if (!m_first)
            {
                m_held = false;
                return;
            }

            ready = m_first;
            m_first = m_first->m_next;
            if (!m_first)
                m_last = nullptr;
            --m_waiters;
        }

        Jobs::JobSystem::Get().SignalWaitingFiber(ready->m_entry);
    }


    uint32_t FiberMutex::GetWaiterCount()
    {
        std::lock_guard guard(m_lock);
        return m_waiters;
    }
} // namespace FE::Threading
