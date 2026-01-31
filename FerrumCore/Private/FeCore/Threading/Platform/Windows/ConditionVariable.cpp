#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Threading/ConditionVariable.h>
#include <FeCore/Threading/Mutex.h>

namespace FE::Threading
{
    ConditionVariable::ConditionVariable() noexcept
    {
        static_assert(sizeof(m_nativeConditionVariable) >= sizeof(CONDITION_VARIABLE));
        InitializeConditionVariable(reinterpret_cast<PCONDITION_VARIABLE>(m_nativeConditionVariable));
    }


    void ConditionVariable::NotifyOne() noexcept
    {
        WakeConditionVariable(reinterpret_cast<PCONDITION_VARIABLE>(m_nativeConditionVariable));
    }


    void ConditionVariable::NotifyAll() noexcept
    {
        WakeAllConditionVariable(reinterpret_cast<PCONDITION_VARIABLE>(m_nativeConditionVariable));
    }


    void ConditionVariable::Wait(std::unique_lock<Mutex>& lock)
    {
        const auto result =
            SleepConditionVariableCS(reinterpret_cast<PCONDITION_VARIABLE>(m_nativeConditionVariable),
                                     reinterpret_cast<LPCRITICAL_SECTION>(lock.mutex()->m_nativeMutex), INFINITE);
        FE_Verify(result);
    }


    bool ConditionVariable::WaitFor(std::unique_lock<Mutex>& lock, const uint32_t milliseconds)
    {
        const auto result =
            SleepConditionVariableCS(reinterpret_cast<PCONDITION_VARIABLE>(m_nativeConditionVariable),
                                     reinterpret_cast<LPCRITICAL_SECTION>(lock.mutex()->m_nativeMutex), milliseconds);
        if (result)
            return true;

        return GetLastError() != ERROR_TIMEOUT;
    }
} // namespace FE::Threading
