#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Threading/Semaphore.h>

#pragma comment(lib, "synchronization.lib")

namespace FE::Threading
{
    Semaphore::Semaphore(const uint32_t initialValue)
    {
        m_semaphore = initialValue;
    }


    Semaphore::~Semaphore() = default;


    void Semaphore::Acquire()
    {
        for (;;)
        {
            intptr_t originalValue = m_semaphore;
            while (originalValue == 0)
            {
                WaitOnAddress(&m_semaphore, &originalValue, sizeof(m_semaphore), INFINITE);
                originalValue = m_semaphore;
            }

            if (InterlockedCompareExchange64(&m_semaphore, originalValue - 1, originalValue) == originalValue)
                return;
        }
    }


    void Semaphore::Release()
    {
        InterlockedIncrement64(&m_semaphore);
        WakeByAddressSingle(&m_semaphore);
    }


    void Semaphore::Release(const uint32_t count)
    {
        InterlockedAdd64(&m_semaphore, count);
        if (count == 1)
            WakeByAddressSingle(&m_semaphore);
        else
            WakeByAddressAll(&m_semaphore);
    }
} // namespace FE::Threading
