#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Parallel/Thread.h>

namespace FE
{
#if FE_WINDOWS
    namespace
    {
        struct ThreadDataImpl final
        {
            DWORD ID;
            HANDLE hThread;
            uintptr_t pUserData;
            ThreadFunction StartRoutine;
            Memory::PoolAllocator* pPool;
        };

        Memory::PoolAllocator g_ThreadDataPool{ "NativeThreadData", sizeof(ThreadDataImpl), 64 * 1024 };
        SpinLock g_ThreadDataPoolLock;


        inline static ThreadDataImpl* AllocateThreadData()
        {
            const std::lock_guard lock{ g_ThreadDataPoolLock };
            ThreadDataImpl* pResult = Memory::New<ThreadDataImpl>(&g_ThreadDataPool);
            pResult->pPool = &g_ThreadDataPool;
            return pResult;
        }


        struct ThreadDataHolder final
        {
            ThreadDataImpl* pData = nullptr;

            ~ThreadDataHolder()
            {
                if (pData)
                {
                    const std::lock_guard lock{ g_ThreadDataPoolLock };
                    Memory::Delete(pData->pPool, pData, sizeof(ThreadDataImpl));
                    pData = nullptr;
                }
            }
        };

        thread_local ThreadDataHolder g_TLSThreadData;


        static DWORD WINAPI ThreadRoutineImpl(LPVOID lpParam)
        {
            auto* pData = static_cast<ThreadDataImpl*>(lpParam);
            g_TLSThreadData.pData = pData;
            pData->StartRoutine(pData->pUserData);
            return 0;
        }
    } // namespace


    ThreadHandle CreateThread(StringSlice name, ThreadFunction startRoutine, uintptr_t pUserData, ThreadPriority priority,
                              size_t stackSize)
    {
        ThreadDataImpl* pData = AllocateThreadData();

        const HANDLE hThread = ::CreateThread(nullptr, stackSize, &ThreadRoutineImpl, pData, CREATE_SUSPENDED, &pData->ID);
        FE_CORE_ASSERT(hThread, "CreateThread failed");

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

        if (priority != ThreadPriority::Normal)
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

        const ThreadDataImpl data = *reinterpret_cast<ThreadDataImpl*>(thread.Value);
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


    uint64_t GetThreadID(ThreadHandle threadHandle)
    {
        return reinterpret_cast<ThreadDataImpl*>(threadHandle.Value)->ID;
    }
#else
#    error Not implemented :(
#endif
} // namespace FE
