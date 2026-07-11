#pragma once
#include <Core/Base/BaseTypes.h>
#include <Core/Env/Environment.h>
#include <Core/Jobs/Base.h>
#include <Core/Memory/LinearAllocator.h>

namespace FE::Jobs
{
    struct Graph final
    {
        explicit Graph(Env::Name name, FiberAffinityMask affinity = FiberAffinityMask::kAll,
                       Priority priority = Priority::kNormal);
        ~Graph();

        Graph(const Graph&) = delete;
        Graph& operator=(const Graph&) = delete;

        Graph(Graph&& other) noexcept;
        Graph& operator=(Graph&& other) noexcept;

        void* Allocate(const size_t byteSize, const size_t byteAlignment)
        {
            return m_allocator.do_allocate(byteSize, byteAlignment);
        }

        template<class TFunctor>
        Rc<WaitGroup> Dispatch(const Env::Name name, TFunctor&& functor)
        {
            return Dispatch<TFunctor>(name, festd::span<WaitGroup* const>{}, std::forward<TFunctor>(functor));
        }

        template<class TFunctor>
        Rc<WaitGroup> Dispatch(const Env::Name name, const std::initializer_list<WaitGroup*> prerequisites, TFunctor&& functor)
        {
            return Dispatch<TFunctor>(name, festd::span(prerequisites), std::forward<TFunctor>(functor));
        }

        template<class TFunctor>
        Rc<WaitGroup> Dispatch(const Env::Name name, const festd::span<WaitGroup* const> prerequisites, TFunctor&& functor)
        {
            using FunctorType = std::decay_t<TFunctor>;
            FunctorType* funcPtr = Memory::New<FunctorType>(&m_allocator, std::forward<FunctorType>(functor));
            const TaskFunction taskFunction = [](void* data) {
                (*static_cast<FunctorType*>(data))();
                static_cast<FunctorType*>(data)->~FunctorType();
            };

            return DispatchJobImpl(name, prerequisites, taskFunction, funcPtr);
        }

        template<class TFunctor>
        Rc<WaitGroup> Dispatch(const Env::Name name, const std::initializer_list<Rc<WaitGroup>> prerequisites, TFunctor&& functor)
        {
            return Dispatch<TFunctor>(name, festd::span(prerequisites), std::forward<TFunctor>(functor));
        }

        template<class TFunctor>
        Rc<WaitGroup> Dispatch(const Env::Name name, const festd::span<const Rc<WaitGroup>> prerequisites, TFunctor&& functor)
        {
            using FunctorType = std::decay_t<TFunctor>;
            FunctorType* funcPtr = Memory::New<FunctorType>(&m_allocator, std::forward<FunctorType>(functor));
            const TaskFunction taskFunction = [](void* data) {
                (*static_cast<FunctorType*>(data))();
                static_cast<FunctorType*>(data)->~FunctorType();
            };

            return DispatchJobImpl(name, prerequisites, taskFunction, funcPtr);
        }

        template<class TFunctor>
        void InvokeOnCompletion(TFunctor&& functor)
        {
            using FunctorType = std::decay_t<TFunctor>;

            FE_Assert(m_completionCallback == nullptr);
            FE_Assert(m_completionCallbackData == nullptr);

            FunctorType* funcPtr = Memory::New<FunctorType>(&m_allocator, std::forward<FunctorType>(functor));
            const TaskFunction taskFunction = [](void* data) {
                (*static_cast<FunctorType*>(data))();
                static_cast<FunctorType*>(data)->~FunctorType();
            };

            m_completionCallback = taskFunction;
            m_completionCallbackData = funcPtr;
        }

        //! @brief Invalidate the job graph and dispatch a cleanup task to free all memory upon completion.
        //!
        //! @return WaitGroup that will be signaled when all tasks are completed and graph memory is freed.
        Rc<WaitGroup> Detach();

        //! @brief Wait for all jobs in the graph to complete.
        //!
        //! This function can only be called from a fiber.
        void Wait();

        [[nodiscard]] bool IsEmpty() const
        {
            return m_jobCount == 0;
        }

        friend void swap(Graph& lhs, Graph& rhs) noexcept;

    private:
        struct JobImpl;
        struct JobRecord;

        using TaskFunction = void (*)(void* data);

        festd::span<WaitGroup* const> MakeAllWaitGroupsArray();
        void CleanUp();

        Rc<WaitGroup> DispatchJobImpl(Env::Name name, festd::span<WaitGroup* const> prerequisites, TaskFunction taskFunction,
                                      void* data);

        Rc<WaitGroup> DispatchJobImpl(Env::Name name, festd::span<const Rc<WaitGroup>> prerequisites, TaskFunction taskFunction,
                                      void* data);

        Env::Name m_name;
        FiberAffinityMask m_affinity = FiberAffinityMask::kNone;
        Priority m_priority = Priority::kNormal;

        TaskFunction m_completionCallback = nullptr;
        void* m_completionCallbackData = nullptr;

        bool m_isValid = true;
        uint32_t m_jobCount = 0;
        JobRecord* m_jobRecords = nullptr;
        Memory::LinearAllocator m_allocator;
    };
} // namespace FE::Jobs
