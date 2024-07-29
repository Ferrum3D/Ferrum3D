#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Parallel/Mutex.h>

namespace FE
{
    Mutex::Mutex(uint32_t spinCount) noexcept
    {
        InitializeCriticalSectionEx(reinterpret_cast<LPCRITICAL_SECTION>(&m_NativeMutex), spinCount, 0);
    }


    Mutex::~Mutex()
    {
        DeleteCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_NativeMutex));
    }


    void Mutex::lock() noexcept
    {
        EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_NativeMutex));
    }


    bool Mutex::try_lock()
    {
        return TryEnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_NativeMutex));
    }


    void Mutex::unlock()
    {
        LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&m_NativeMutex));
    }
} // namespace FE
