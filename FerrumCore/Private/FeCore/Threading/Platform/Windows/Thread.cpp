#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Threading/Thread.h>
#include <FeCore/Threading/ThreadingPrivate.h>

namespace FE::Threading
{
    namespace
    {
        struct ThreadingState final
        {
            SpinLock m_lock;
            uint64_t m_mainThreadID = 0;
            Memory::Pool<NativeThreadData> m_threadDataAllocator{ "NativeThreadData", 64 * 1024 };
        };

        ThreadingState* GThreadingState;


        NativeThreadData* AllocateThreadData()
        {
            const std::lock_guard lock{ GThreadingState->m_lock };
            return GThreadingState->m_threadDataAllocator.New();
        }


        struct ThreadDataHolder final
        {
            NativeThreadData* m_data = nullptr;

            ~ThreadDataHolder()
            {
                if (m_data)
                {
                    const std::lock_guard lock{ GThreadingState->m_lock };
                    GThreadingState->m_threadDataAllocator.Delete(m_data);
                    m_data = nullptr;
                }
            }
        };

        thread_local ThreadDataHolder GTLSThreadData;


        DWORD WINAPI ThreadRoutineImpl(const LPVOID lpParam)
        {
            auto* pData = static_cast<NativeThreadData*>(lpParam);
            GTLSThreadData.m_data = pData;
            pData->m_startRoutine(pData->m_userData);
            return 0;
        }
    } // namespace


    void Internal::Init(std::pmr::memory_resource* allocator)
    {
        FE_CoreAssert(GThreadingState == nullptr, "Threading already initialized");
        GThreadingState = Memory::New<ThreadingState>(allocator);
        GThreadingState->m_mainThreadID = ::GetCurrentThreadId();
    }


    void Internal::Shutdown()
    {
        FE_CoreAssert(GThreadingState != nullptr, "Threading not initialized");
        GThreadingState->~ThreadingState();
        GThreadingState = nullptr;
    }


    ThreadHandle CreateThread(const festd::string_view name, const ThreadFunction startRoutine, const uintptr_t pUserData,
                              Priority priority, const size_t stackSize)
    {
        FE_PROFILER_ZONE();

        NativeThreadData* data = AllocateThreadData();

        DWORD threadID;
        const HANDLE hThread = ::CreateThread(nullptr, stackSize, &ThreadRoutineImpl, data, CREATE_SUSPENDED, &threadID);
        FE_CoreAssert(hThread, "CreateThread failed");

        data->m_id = threadID;
        data->m_priority = priority;
        data->m_threadHandle = hThread;
        data->m_userData = pUserData;
        data->m_startRoutine = startRoutine;

        if (Build::IsDevelopment())
        {
            const Str::Utf8ToUtf16 wideName{ name.data(), name.size() };

            if (const HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll"))
            {
                using SetThreadDescriptionProc = HRESULT(WINAPI*)(HANDLE, PCWSTR);
                if (const auto setThreadDescription = reinterpret_cast<void*>(GetProcAddress(hKernel32, "SetThreadDescription")))
                    reinterpret_cast<SetThreadDescriptionProc>(setThreadDescription)(hThread, wideName.ToWideString());
            }
        }

        if (priority != Priority::kNormal)
        {
            const BOOL priorityRes = SetThreadPriority(hThread, static_cast<int>(priority));
            FE_CoreAssert(priorityRes, "SetThreadPriority failed");
        }

        FE_CoreVerify(ResumeThread(hThread) != Constants::kMaxValue<DWORD>);

        return ThreadHandle{ reinterpret_cast<uint64_t>(data) };
    }


    void CloseThread(ThreadHandle& thread)
    {
        FE_PROFILER_ZONE();

        if (thread.m_value == 0)
            return;

        const NativeThreadData data = *reinterpret_cast<NativeThreadData*>(thread.m_value);
        FE_CoreVerify(WaitForSingleObject(data.m_threadHandle, INFINITE) == WAIT_OBJECT_0);

        const BOOL closeRes = CloseHandle(data.m_threadHandle);
        FE_CoreAssert(closeRes, "CloseHandle failed on thread");
        thread.Reset();
    }


    void Sleep(const uint32_t milliseconds)
    {
        FE_PROFILER_ZONE();
        ::Sleep(milliseconds);
    }


    uint64_t GetCurrentThreadID()
    {
        return ::GetCurrentThreadId();
    }


    uint64_t GetMainThreadID()
    {
        return GThreadingState->m_mainThreadID;
    }
} // namespace FE::Threading
