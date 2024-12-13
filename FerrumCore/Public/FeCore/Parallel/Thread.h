#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    typedef void (*ThreadFunction)(uintptr_t);


    namespace Threading
    {
        enum class Priority : int32_t
        {
            kLowest = -2,
            kBelowNormal = -1,
            kNormal = 0,
            kAboveNormal = 1,
            kHighest = 2,
        };
    } // namespace Threading


#if FE_PLATFORM_WINDOWS
    struct NativeThreadData final
    {
        uint32_t ID;
        Threading::Priority Priority;
        void* hThread;
        uintptr_t pUserData;
        ThreadFunction StartRoutine;
    };
#else
#    error Not implemented :(
#endif


    struct ThreadHandle final : TypedHandle<ThreadHandle, uint64_t, 0>
    {
    };


    ThreadHandle CreateThread(StringSlice name, ThreadFunction startRoutine, uintptr_t pUserData = 0,
                              Threading::Priority priority = Threading::Priority::kNormal, size_t stackSize = 0);
    void CloseThread(ThreadHandle& thread);

    uint64_t GetCurrentThreadID();


    inline static const NativeThreadData& GetNativeThreadData(ThreadHandle threadHandle)
    {
        return *reinterpret_cast<const NativeThreadData*>(threadHandle.m_value);
    }


    inline static uint64_t GetThreadID(ThreadHandle threadHandle)
    {
        return GetNativeThreadData(threadHandle).ID;
    }
} // namespace FE
