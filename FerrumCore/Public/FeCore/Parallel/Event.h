#pragma once
#include <FeCore/Memory/PoolAllocator.h>

namespace FE::Threading
{
    class Event final
    {
        uintptr_t m_NativeEvent = 0;

        Event(uintptr_t nativeEvent)
            : m_NativeEvent(nativeEvent)
        {
        }

    public:
        Event() = default;

        ~Event()
        {
            Close();
        }

        Event(Event&& other) noexcept
            : m_NativeEvent(other.m_NativeEvent)
        {
            other.m_NativeEvent = 0;
        }

        Event& operator=(Event&& other) noexcept
        {
            Close();
            m_NativeEvent = other.m_NativeEvent;
            other.m_NativeEvent = 0;
            return *this;
        }

        void Send();
        void Wait();
        void Close();

        static Event CreateAutoReset(bool initialState = false);
        static Event CreateManualReset(bool initialState = false);
    };
} // namespace FE::Threading
