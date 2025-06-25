#pragma once
#include <FeCore/Threading/SpinLock.h>
#include <mutex>

namespace FE
{
    struct ConcurrentQueue final
    {
        struct Node
        {
            Node* m_next;
        };

        ConcurrentQueue() = default;

        void Enqueue(Node* node)
        {
            std::lock_guard lock{ m_lock };
            node->m_next = nullptr;
            if (m_tail)
                m_tail->m_next = node;
            else
                m_head = node;
            m_tail = node;
        }

        void PushFront(Node* node)
        {
            std::lock_guard lock{ m_lock };
            node->m_next = m_head;
            m_head = node;
        }

        Node* TryDequeue()
        {
            std::unique_lock lock{ m_lock };
            if (m_head)
            {
                Node* node = m_head;
                m_head = m_head->m_next;
                if (!m_head)
                    m_tail = nullptr;

                return node;
            }

            return nullptr;
        }

    private:
        Node* m_head = nullptr;
        Node* m_tail = nullptr;
        Threading::SpinLock m_lock;
    };


    struct ConcurrentOnceConsumedQueue final
    {
        struct Node
        {
            Node* m_next;
        };

        void Enqueue(Node* node)
        {
            Node* currentTop = m_top.load(std::memory_order_relaxed);
            for (;;)
            {
                node->m_next = currentTop;
                if (m_top.compare_exchange_weak(currentTop, node, std::memory_order_release, std::memory_order_relaxed))
                    break;
            }
        }

        [[nodiscard]] Node* DequeueAll()
        {
            return m_top.exchange(nullptr, std::memory_order_relaxed);
        }

        [[nodiscard]] bool Empty() const
        {
            return !m_top.load(std::memory_order_relaxed);
        }

    private:
        std::atomic<Node*> m_top = nullptr;
    };
} // namespace FE
