#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Threading/Mutex.h>

namespace FE::Threading
{
    Mutex::Mutex(const uint32_t spinCount) noexcept
    {
        FE_PROFILER_ZONE();

        static_assert(sizeof(m_nativeMutex) >= sizeof(CRITICAL_SECTION));
        InitializeCriticalSectionEx(reinterpret_cast<LPCRITICAL_SECTION>(m_nativeMutex), spinCount, 0);
    }


    Mutex::~Mutex()
    {
        DeleteCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(m_nativeMutex));
    }


    void Mutex::lock() noexcept
    {
        EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(m_nativeMutex));
    }


    bool Mutex::try_lock()
    {
        return TryEnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(m_nativeMutex));
    }


    void Mutex::unlock()
    {
        LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(m_nativeMutex));
    }
} // namespace FE::Threading
