#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Parallel/Semaphore.h>

namespace FE
{
    Semaphore::Semaphore(UInt32 initialValue)
    {
        m_NativeSemaphore = CreateSemaphore(nullptr, initialValue, 0xfff, nullptr);
    }

    Semaphore::~Semaphore()
    {
        CloseHandle(m_NativeSemaphore);
    }

    void Semaphore::Acquire()
    {
        WaitForSingleObject(m_NativeSemaphore, static_cast<DWORD>(-1));
    }

    void Semaphore::Release(UInt32 count)
    {
        ReleaseSemaphore(m_NativeSemaphore, count, nullptr);
    }
} // namespace FE
