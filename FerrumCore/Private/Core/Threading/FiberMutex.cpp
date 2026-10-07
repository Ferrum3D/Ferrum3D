#include <Core/Threading/FiberMutex.h>

namespace FE::Threading
{
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
            waiter.m_ready = WaitGroup::Create();
            if (m_last)
                m_last->m_next = &waiter;
            else
                m_first = &waiter;
            m_last = &waiter;
            ++m_waiters;
        }
        waiter.m_ready->Wait();
    }


    void FiberMutex::unlock()
    {
        Rc<WaitGroup> ready;
        {
            std::lock_guard guard(m_lock);
            FE_Assert(m_held);
            if (!m_first)
            {
                m_held = false;
                return;
            }
            ready = m_first->m_ready;
            m_first = m_first->m_next;
            if (!m_first)
                m_last = nullptr;
            --m_waiters;
        }
        ready->Signal();
    }


    uint32_t FiberMutex::GetWaiterCount()
    {
        std::lock_guard guard(m_lock);
        return m_waiters;
    }
} // namespace FE::Threading
