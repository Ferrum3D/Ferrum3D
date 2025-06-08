#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Modules/EnvironmentPrivate.h>
#include <FeCore/Threading/Platform/ThreadingInternal.h>
#include <FeCore/Threading/Thread.h>

namespace FE::Threading
{
    namespace
    {
        NativeThreadData* AllocateThreadData()
        {
            Env::Internal::SharedState& state = Env::Internal::SharedState::Get();
            const std::lock_guard lock{ state.m_lock };
            return Memory::New<NativeThreadData>(&state.m_threadDataAllocator);
        }


        struct ThreadDataHolder final
        {
            NativeThreadData* pData = nullptr;

            ~ThreadDataHolder()
            {
                if (pData)
                {
                    Env::Internal::SharedState& state = Env::Internal::SharedState::Get();
                    const std::lock_guard lock{ state.m_lock };
                    Memory::Delete(&state.m_threadDataAllocator, pData, sizeof(NativeThreadData));
                    pData = nullptr;
                }
            }
        };

        thread_local ThreadDataHolder GTLSThreadData;


        DWORD WINAPI ThreadRoutineImpl(const LPVOID lpParam)
        {
            auto* pData = static_cast<NativeThreadData*>(lpParam);
            GTLSThreadData.pData = pData;
            pData->m_startRoutine(pData->m_userData);
            return 0;
        }
    } // namespace


    void Internal::Init()
    {
        Env::Internal::SharedState& state = Env::Internal::SharedState::Get();
        state.m_threadDataAllocator.Initialize("NativeThreadData", sizeof(NativeThreadData), 64 * 1024);
    }


    ThreadHandle CreateThread(const festd::string_view name, const ThreadFunction startRoutine, const uintptr_t pUserData,
                              Priority priority, const size_t stackSize)
    {
        FE_PROFILER_ZONE();

        NativeThreadData* pData = AllocateThreadData();

        DWORD threadID;
        const HANDLE hThread = ::CreateThread(nullptr, stackSize, &ThreadRoutineImpl, pData, CREATE_SUSPENDED, &threadID);
        FE_CoreAssert(hThread, "CreateThread failed");

        pData->m_id = threadID;
        pData->m_priority = priority;
        pData->m_threadHandle = hThread;
        pData->m_userData = pUserData;
        pData->m_startRoutine = startRoutine;

        WCHAR wideName[64];
        const int32_t wideNameLength =
            MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), wideName, festd::size(wideName));
        wideName[wideNameLength] = 0;

        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        if (hKernel32)
        {
            using SetThreadDescriptionProc = HRESULT(WINAPI*)(HANDLE, PCWSTR);
            const SetThreadDescriptionProc setThreadDescription =
                reinterpret_cast<SetThreadDescriptionProc>(GetProcAddress(hKernel32, "SetThreadDescription"));
            if (setThreadDescription)
                setThreadDescription(hThread, wideName);
        }

        if (priority != Priority::kNormal)
        {
            const BOOL priorityRes = SetThreadPriority(hThread, static_cast<int>(priority));
            FE_CoreAssert(priorityRes, "SetThreadPriority failed");
        }

        FE_CoreVerify(ResumeThread(hThread) != Constants::kMaxValue<DWORD>);

        return ThreadHandle{ reinterpret_cast<uint64_t>(pData) };
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


    uint64_t GetCurrentThreadID()
    {
        return ::GetCurrentThreadId();
    }


    uint64_t GetMainThreadID()
    {
        return Env::Internal::SharedState::Get().m_mainThreadId;
    }
} // namespace FE::Threading
