#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    typedef void (*ThreadFunction)(uintptr_t);


    enum class ThreadPriority : int32_t
    {
        Lowest = -2,
        BelowNormal = -1,
        Normal = 0,
        AboveNormal = 1,
        Highest = 2,
    };


#if FE_WINDOWS
    struct NativeThreadData final
    {
        uint32_t ID;
        ThreadPriority Priority;
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
                              ThreadPriority priority = ThreadPriority::Normal, size_t stackSize = 0);
    void CloseThread(ThreadHandle& thread);

    uint64_t GetCurrentThreadID();


    inline static const NativeThreadData& GetNativeThreadData(ThreadHandle threadHandle)
    {
        return *reinterpret_cast<const NativeThreadData*>(threadHandle.Value);
    }


    inline static uint64_t GetThreadID(ThreadHandle threadHandle)
    {
        return GetNativeThreadData(threadHandle).ID;
    }
} // namespace FE
