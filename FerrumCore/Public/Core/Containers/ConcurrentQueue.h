#pragma once
#include <Core/Threading/SpinLock.h>
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

        [[nodiscard]] Node* TryDequeue()
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

        [[nodiscard]] bool Empty() const
        {
            std::unique_lock lock{ m_lock };
            return m_head == nullptr;
        }

    private:
        Node* m_head = nullptr;
        Node* m_tail = nullptr;

        mutable Threading::SpinLock m_lock;
    };


    struct ConcurrentOnceConsumedQueue final
    {
        struct Node
        {
            Node* m_next;
        };

        void Enqueue(Node* node)
        {
            Node* currentTop = m_top.load(std::memory_order_acquire);
            for (;;)
            {
                node->m_next = currentTop;
                if (m_top.compare_exchange_weak(currentTop, node, std::memory_order_acq_rel, std::memory_order_acquire))
                    break;
            }
        }

        [[nodiscard]] Node* DequeueAll()
        {
            return m_top.exchange(nullptr, std::memory_order_acquire);
        }

        [[nodiscard]] bool Empty() const
        {
            return !m_top.load(std::memory_order_acquire);
        }

    private:
        std::atomic<Node*> m_top = nullptr;
    };
} // namespace FE
