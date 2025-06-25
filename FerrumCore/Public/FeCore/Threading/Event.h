#pragma once
#include <festd/base.h>

namespace FE::Threading
{
    struct Event final
    {
        Event() = default;

        ~Event()
        {
            Close();
        }

        Event(Event&& other) noexcept
            : m_nativeEvent(other.m_nativeEvent)
        {
            other.m_nativeEvent = 0;
        }

        Event& operator=(Event&& other) noexcept
        {
            festd::swap(m_nativeEvent, other.m_nativeEvent);
            other.Close();
            return *this;
        }

        void Send();
        void Reset();
        void Wait();
        void Close();

        static Event CreateAutoReset(bool initialState = false);
        static Event CreateManualReset(bool initialState = false);

    private:
        uintptr_t m_nativeEvent = 0;

        explicit Event(const uintptr_t nativeEvent)
            : m_nativeEvent(nativeEvent)
        {
        }
    };
} // namespace FE::Threading
