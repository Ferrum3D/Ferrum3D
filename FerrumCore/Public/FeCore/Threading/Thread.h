#pragma once
#include <FeCore/Base/Base.h>
#include <festd/string.h>

namespace FE::Threading
{
    typedef void (*ThreadFunction)(uintptr_t);


    enum class Priority : int32_t
    {
        kLowest = -2,
        kBelowNormal = -1,
        kNormal = 0,
        kAboveNormal = 1,
        kHighest = 2,
    };


#if FE_PLATFORM_WINDOWS
    struct NativeThreadData final
    {
        uint32_t m_id;
        Priority m_priority;
        void* m_threadHandle;
        uintptr_t m_userData;
        ThreadFunction m_startRoutine;
    };
#else
#    error Not implemented :(
#endif


    struct ThreadHandle final : TypedHandle<ThreadHandle, uint64_t, 0>
    {
    };


    ThreadHandle CreateThread(festd::string_view name, ThreadFunction startRoutine, uintptr_t pUserData = 0,
                              Priority priority = Priority::kNormal, size_t stackSize = 0);
    void CloseThread(ThreadHandle& thread);

    uint64_t GetCurrentThreadID();
    uint64_t GetMainThreadID();

    inline bool IsMainThread()
    {
        return GetCurrentThreadID() == GetMainThreadID();
    }


    inline const NativeThreadData& GetNativeThreadData(const ThreadHandle threadHandle)
    {
        return *reinterpret_cast<const NativeThreadData*>(threadHandle.m_value);
    }


    inline uint64_t GetThreadID(const ThreadHandle threadHandle)
    {
        return GetNativeThreadData(threadHandle).m_id;
    }
} // namespace FE::Threading
