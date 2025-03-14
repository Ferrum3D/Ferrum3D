#pragma once
#include <FeCore/Threading/SpinLock.h>

namespace FE
{
    struct ConcurrentQueueNode
    {
        ConcurrentQueueNode* m_next = nullptr;
    };


    struct ConcurrentQueue final
    {
        ConcurrentQueue() = default;

        void Enqueue(ConcurrentQueueNode* node)
        {
            std::lock_guard lock{ m_lock };
            node->m_next = nullptr;
            if (m_tail)
                m_tail->m_next = node;
            else
                m_head = node;
            m_tail = node;
        }

        void PushFront(ConcurrentQueueNode* node)
        {
            std::lock_guard lock{ m_lock };
            node->m_next = m_head;
            m_head = node;
        }

        ConcurrentQueueNode* TryDequeue()
        {
            std::unique_lock lock{ m_lock };
            if (m_head)
            {
                ConcurrentQueueNode* node = m_head;
                m_head = m_head->m_next;
                if (!m_head)
                    m_tail = nullptr;

                return node;
            }

            return nullptr;
        }

    private:
        ConcurrentQueueNode* m_head = nullptr;
        ConcurrentQueueNode* m_tail = nullptr;
        Threading::SpinLock m_lock;
    };
} // namespace FE
