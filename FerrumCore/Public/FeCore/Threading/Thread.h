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

    void Sleep(uint32_t milliseconds);

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


    struct Thread final
    {
        Thread() = default;

        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;

        Thread(Thread&& other) noexcept
            : m_handle(other.m_handle)
        {
            other.m_handle.Reset();
            other.m_function = nullptr;
            other.m_functionDestructor = nullptr;
        }

        Thread& operator=(Thread&& other) noexcept
        {
            if (this == &other)
                return *this;

            Join();
            m_handle = other.m_handle;
            other.m_handle.Reset();
            other.m_function = nullptr;
            other.m_functionDestructor = nullptr;
            return *this;
        }

        ~Thread()
        {
            Join();
        }

        template<class TFunc>
        explicit Thread(const festd::string_view name, TFunc&& func, const Priority priority = Priority::kNormal,
                        const size_t stackSize = 0)
        {
            Start(name, std::forward<TFunc>(func), priority, stackSize);
        }

        template<class TFunc>
        void Start(const festd::string_view name, TFunc&& func, const Priority priority = Priority::kNormal,
                   const size_t stackSize = 0)
        {
            FE_Assert(!m_handle.IsValid(), "Thread already running");
            m_function = Memory::DefaultNew<TFunc>(std::forward<TFunc>(func));
            m_functionDestructor = [](void* function) {
                static_cast<TFunc*>(function)->~TFunc();
            };
            m_handle = CreateThread(name, &Thread::Trampoline<TFunc>, reinterpret_cast<uintptr_t>(this), priority, stackSize);
        }

        void Join()
        {
            if (m_handle.IsValid())
            {
                CloseThread(m_handle);
                DestroyFunction();
            }
            else
            {
                FE_Assert(m_function == nullptr);
                FE_Assert(m_functionDestructor == nullptr);
            }
        }

        [[nodiscard]] bool Joinable() const
        {
            return m_handle.IsValid();
        }

    private:
        ThreadHandle m_handle;
        void* m_function = nullptr;
        void (*m_functionDestructor)(void*) = nullptr;

        void DestroyFunction()
        {
            if (m_function)
            {
                m_functionDestructor(m_function);
                Memory::DefaultFree(m_function);
            }

            m_function = nullptr;
            m_functionDestructor = nullptr;
        }

        template<class TFunc>
        static void Trampoline(const uintptr_t userData)
        {
            auto* thread = reinterpret_cast<Thread*>(userData);
            (*static_cast<TFunc*>(thread->m_function))();
            thread->DestroyFunction();
        }
    };
} // namespace FE::Threading
