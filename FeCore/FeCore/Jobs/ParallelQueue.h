#pragma once
#include <Utils/CoreUtils.h>
#include <mutex>
#include <queue>
#include <shared_mutex>

namespace FE
{
    template<class T>
    class FeParallelQueue
    {
        std::queue<T> m_Queue;
        std::shared_mutex m_Mutex;

    public:
        /**
		* @brief Push to the queue.
		* @param val What to push.
		*/
        void Push(const T& val)
        {
            std::unique_lock lk(m_Mutex);
            m_Queue.push(val);
        }

        /**
		* @brief Clear the queue.
		*/
        void Clear()
        {
            std::unique_lock lk(m_Mutex);
            while (!m_Queue.empty())
            {
                m_Queue.pop();
            }
        }

        /**
		* @brief Try pop from the queue.
		* @param val Reference to variable. This variable will be set to the front of the queue.
		* @return true if queue was not empty and a value can be retrieved.
		*/
        bool TryPop(T& val)
        {
            std::unique_lock lk(m_Mutex);
            if (m_Queue.empty())
            {
                return false;
            }
            val = m_Queue.front();
            m_Queue.pop();
            return true;
        }

        /**
		* @brief Check if the queue is empty
		* @return true if the queue is empty.
		*/
        bool IsEmpty()
        {
            std::shared_lock lk(m_Mutex);
            return m_Queue.empty();
        }
    };
} // namespace FE
