#pragma once
#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Parallel/Interlocked.h>

#if FE_WINDOWS
using TNativeMutex = RTL_CRITICAL_SECTION;
#else
#   error Platform not supported yet
#endif

namespace FE
{
    class Mutex final
    {
        TNativeMutex m_NativeMutex;

    public:
        Mutex(const Mutex&) = delete;
        Mutex& operator=(const Mutex&) = delete;

        FE_FINLINE Mutex() noexcept // NOLINT
        {
            InitializeCriticalSection(&m_NativeMutex);
        }

        FE_FINLINE ~Mutex()
        {
            DeleteCriticalSection(&m_NativeMutex);
        }

        FE_FINLINE void Lock() noexcept
        {
            EnterCriticalSection(&m_NativeMutex);
        }

        bool TryLock()
        {
            return TryEnterCriticalSection(&m_NativeMutex);
        }

        void Unlock()
        {
            LeaveCriticalSection(&m_NativeMutex);
        }
    };
}
