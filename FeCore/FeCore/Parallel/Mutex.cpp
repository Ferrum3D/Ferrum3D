#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Parallel/Mutex.h>

namespace FE
{
    Mutex::Mutex() noexcept // NOLINT
    {
        InitializeCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_NativeMutex));
    }

    Mutex::~Mutex()
    {
        DeleteCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_NativeMutex));
    }

    void Mutex::Lock() noexcept
    {
        EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_NativeMutex));
    }

    bool Mutex::TryLock()
    {
        return TryEnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_NativeMutex));
    }

    void Mutex::Unlock()
    {
        LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_NativeMutex));
    }
} // namespace FE
