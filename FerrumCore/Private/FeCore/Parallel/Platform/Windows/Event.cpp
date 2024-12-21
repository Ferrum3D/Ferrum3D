#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/Parallel/Event.h>

namespace FE::Threading
{
    void Event::Send()
    {
        FE_Verify(SetEvent(reinterpret_cast<HANDLE>(m_nativeEvent)));
    }


    void Event::Wait()
    {
        FE_Verify(WaitForSingleObject(reinterpret_cast<HANDLE>(m_nativeEvent), INFINITE) == WAIT_OBJECT_0);
    }


    void Event::Close()
    {
        if (m_nativeEvent == 0)
            return;

        CloseHandle(reinterpret_cast<HANDLE>(m_nativeEvent));
        m_nativeEvent = 0;
    }


    Event Event::CreateAutoReset(bool initialState)
    {
        return reinterpret_cast<uintptr_t>(CreateEventW(nullptr, FALSE, initialState ? TRUE : FALSE, nullptr));
    }


    Event Event::CreateManualReset(bool initialState)
    {
        return reinterpret_cast<uintptr_t>(CreateEventW(nullptr, TRUE, initialState ? TRUE : FALSE, nullptr));
    }
} // namespace FE::Threading
