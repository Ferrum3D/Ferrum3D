#pragma once
#include <Core/Base/BaseTypes.h>
#include <Core/Jobs/Base.h>
#include <Core/Memory/LinearAllocator.h>

namespace FE
{
    struct TaskGraph final
    {
        explicit TaskGraph(Env::Name name, IJobSystem* jobSystem, FiberAffinityMask affinity = FiberAffinityMask::kAllForeground,
                           JobPriority priority = JobPriority::kNormal);
        ~TaskGraph();

        TaskGraph(const TaskGraph&) = delete;
        TaskGraph& operator=(const TaskGraph&) = delete;

        TaskGraph(TaskGraph&& other) noexcept;
        TaskGraph& operator=(TaskGraph&& other) noexcept;

        void* Allocate(const size_t byteSize, const size_t byteAlignment)
        {
            return m_allocator.do_allocate(byteSize, byteAlignment);
        }

        template<class TFunctor>
        Rc<WaitGroup> Schedule(const Env::Name name, TFunctor&& functor)
        {
            return Schedule<TFunctor>(name, festd::span<WaitGroup* const>{}, std::forward<TFunctor>(functor));
        }

        template<class TFunctor>
        Rc<WaitGroup> Schedule(const Env::Name name, const std::initializer_list<WaitGroup*> prerequisites, TFunctor&& functor)
        {
            return Schedule<TFunctor>(name, festd::span(prerequisites), std::forward<TFunctor>(functor));
        }

        template<class TFunctor>
        Rc<WaitGroup> Schedule(const Env::Name name, const festd::span<WaitGroup* const> prerequisites, TFunctor&& functor)
        {
            using FunctorType = std::decay_t<TFunctor>;
            FunctorType* funcPtr = Memory::New<FunctorType>(&m_allocator, std::forward<FunctorType>(functor));
            const TaskFunction taskFunction = [](void* data) {
                (*static_cast<FunctorType*>(data))();
                static_cast<FunctorType*>(data)->~FunctorType();
            };

            return ScheduleTaskImpl(name, prerequisites, taskFunction, funcPtr);
        }

        //! @brief Invalidate the TaskGraph and schedule a cleanup task to free all memory upon completion.
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

        friend void swap(TaskGraph& lhs, TaskGraph& rhs) noexcept;

    private:
        struct JobImpl;
        struct JobRecord;

        using TaskFunction = void (*)(void* data);

        festd::span<WaitGroup* const> MakeAllWaitGroupsArray();
        void CleanUp();

        Rc<WaitGroup> ScheduleTaskImpl(Env::Name name, festd::span<WaitGroup* const> prerequisites, TaskFunction taskFunction,
                                       void* data);

        Env::Name m_name;

        IJobSystem* m_jobSystem = nullptr;
        FiberAffinityMask m_affinity = FiberAffinityMask::kNone;
        JobPriority m_priority = JobPriority::kNormal;

        bool m_isValid = false;
        uint32_t m_jobCount = 0;
        JobRecord* m_jobRecords = nullptr;
        Memory::LinearAllocator m_allocator;
    };
} // namespace FE
