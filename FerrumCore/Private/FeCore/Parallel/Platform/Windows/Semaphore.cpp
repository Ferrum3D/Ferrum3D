#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Parallel/Semaphore.h>

namespace FE::Threading
{
    Semaphore::Semaphore(uint32_t initialValue)
    {
        m_nativeSemaphore = CreateSemaphore(nullptr, initialValue, 0xfff, nullptr);
    }

    Semaphore::~Semaphore()
    {
        CloseHandle(m_nativeSemaphore);
    }

    void Semaphore::Acquire()
    {
        WaitForSingleObject(m_nativeSemaphore, static_cast<DWORD>(-1));
    }

    void Semaphore::Release(uint32_t count)
    {
        ReleaseSemaphore(m_nativeSemaphore, count, nullptr);
    }
} // namespace FE::Threading
