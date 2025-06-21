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


    //! @brief A tagged pointer that can be used in lock-free data structures to solve the ABA problem.
    template<class T>
    struct ConcurrentLink final
    {
        uint64_t m_ptr : 48;
        uint64_t m_counter : 16;

        ConcurrentLink(std::nullptr_t)
            : m_ptr(0)
            , m_counter(0)
        {
        }

        ConcurrentLink(T* ptr, const uint16_t counter)
            : m_ptr(reinterpret_cast<uint64_t>(ptr))
            , m_counter(counter)
        {
        }

        [[nodiscard]] FE_ALWAYS_INLINE T* GetPtr() const
        {
            return reinterpret_cast<T*>(m_ptr);
        }

        FE_ALWAYS_INLINE explicit operator bool() const
        {
            return m_ptr != 0;
        }
    };


    struct ConcurrentStack final
    {
        struct Node
        {
            Node* m_next;
        };

        void Push(Node* node)
        {
            auto currentTop = m_top.load(std::memory_order_relaxed);
            for (;;)
            {
                const ConcurrentLink newTop{ node, static_cast<uint16_t>(currentTop.m_counter + 1u) };
                node->m_next = currentTop.GetPtr();

                if (m_top.compare_exchange_weak(currentTop, newTop, std::memory_order_release, std::memory_order_relaxed))
                    break;
            }
        }

        Node* Pop()
        {
            auto currentTop = m_top.load(std::memory_order_acquire);
            for (;;)
            {
                if (!currentTop)
                    return nullptr;

                const ConcurrentLink newTop{ currentTop.GetPtr()->m_next, static_cast<uint16_t>(currentTop.m_counter + 1u) };
                if (m_top.compare_exchange_weak(currentTop, newTop, std::memory_order_release, std::memory_order_relaxed))
                    return currentTop.GetPtr();
            }
        }

        Node* PopAll()
        {
            auto currentTop = m_top.load(std::memory_order_relaxed);
            for (;;)
            {
                const ConcurrentLink<Node> newTop{ nullptr, static_cast<uint16_t>(currentTop.m_counter + 1u) };
                if (m_top.compare_exchange_weak(currentTop, newTop, std::memory_order_release, std::memory_order_relaxed))
                    return currentTop.GetPtr();
            }
        }

    private:
        std::atomic<ConcurrentLink<Node>> m_top{ nullptr };
        static_assert(decltype(m_top)::is_always_lock_free);
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
