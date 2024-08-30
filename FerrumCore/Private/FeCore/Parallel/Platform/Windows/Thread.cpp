#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Modules/EnvironmentPrivate.h>
#include <FeCore/Parallel/Thread.h>

namespace FE
{
    namespace
    {
        static SpinLock g_ThreadDataPoolLock;


        inline static NativeThreadData* AllocateThreadData()
        {
            const std::lock_guard lock{ g_ThreadDataPoolLock };
            NativeThreadData* pResult = Memory::New<NativeThreadData>(Env::Internal::GetThreadDataPool());
            return pResult;
        }


        struct ThreadDataHolder final
        {
            NativeThreadData* pData = nullptr;

            ~ThreadDataHolder()
            {
                if (pData)
                {
                    const std::lock_guard lock{ g_ThreadDataPoolLock };
                    Memory::Delete(Env::Internal::GetThreadDataPool(), pData, sizeof(NativeThreadData));
                    pData = nullptr;
                }
            }
        };

        thread_local ThreadDataHolder g_TLSThreadData;


        static DWORD WINAPI ThreadRoutineImpl(LPVOID lpParam)
        {
            auto* pData = static_cast<NativeThreadData*>(lpParam);
            g_TLSThreadData.pData = pData;
            pData->StartRoutine(pData->pUserData);
            return 0;
        }
    } // namespace


    ThreadHandle CreateThread(StringSlice name, ThreadFunction startRoutine, uintptr_t pUserData, Threading::Priority priority,
                              size_t stackSize)
    {
        NativeThreadData* pData = AllocateThreadData();

        DWORD threadID;
        const HANDLE hThread = ::CreateThread(nullptr, stackSize, &ThreadRoutineImpl, pData, CREATE_SUSPENDED, &threadID);
        FE_CORE_ASSERT(hThread, "CreateThread failed");

        pData->ID = threadID;
        pData->Priority = priority;
        pData->hThread = hThread;
        pData->pUserData = pUserData;
        pData->StartRoutine = startRoutine;

        WCHAR wideName[64];
        const int32_t wideNameLength =
            MultiByteToWideChar(CP_UTF8, 0, name.Data(), static_cast<int>(name.Size()), wideName, std::size(wideName));
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

        if (priority != Threading::Priority::kNormal)
        {
            const BOOL priorityRes = SetThreadPriority(hThread, static_cast<int>(priority));
            FE_CORE_ASSERT(priorityRes, "SetThreadPriority failed");
        }

        const DWORD resumeRes = ResumeThread(hThread);
        FE_CORE_ASSERT(resumeRes != static_cast<DWORD>(-1), "ResumeThread failed");

        return ThreadHandle{ reinterpret_cast<uint64_t>(pData) };
    }


    void CloseThread(ThreadHandle& thread)
    {
        if (thread.Value == 0)
            return;

        const NativeThreadData data = *reinterpret_cast<NativeThreadData*>(thread.Value);
        const DWORD waitRes = WaitForSingleObject(data.hThread, INFINITE);
        FE_CORE_ASSERT(waitRes == WAIT_OBJECT_0, "WaitForSingleObject failed on thread");

        const BOOL closeRes = CloseHandle(data.hThread);
        FE_CORE_ASSERT(closeRes, "CloseHandle failed on thread");
        thread.Reset();
    }


    uint64_t GetCurrentThreadID()
    {
        return ::GetCurrentThreadId();
    }
} // namespace FE
