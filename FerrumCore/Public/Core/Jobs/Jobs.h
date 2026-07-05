#pragma once
#include <Core/Jobs/JobNode.h>

namespace FE::Jobs
{
    namespace Internal
    {
        template<class TFunctor>
        struct FunctorNode final : public JobNode
        {
            FunctorNode(TFunctor&& functor)
                : m_functor(std::forward<TFunctor>(functor))
            {
            }

            void Execute() override
            {
                m_functor();
                Memory::DefaultDelete(this);
            }

            TFunctor m_functor;
        };
    } // namespace Internal


    void StartJobSystem();
    void StopJobSystem();

    FiberAffinityMask GetAffinityMaskForCurrentThread();


    template<class TFunctor>
    void Dispatch(const FiberAffinityMask affinity, const festd::span<WaitGroup* const> prerequisites, const Priority priority,
                  TFunctor&& functor, WaitGroup* completionWaitGroup = nullptr)
    {
        using JobType = Internal::FunctorNode<TFunctor>;
        JobType* job = Memory::DefaultNew<JobType>(std::forward<TFunctor>(functor));
        if (!prerequisites.empty())
            job->AddPrerequisites(prerequisites);

        job->Dispatch(affinity, completionWaitGroup, priority);
    }

    template<class TFunctor>
    void Dispatch(const FiberAffinityMask affinity, const std::initializer_list<const Rc<WaitGroup>> prerequisites,
                  const Priority priority, TFunctor&& functor, WaitGroup* completionWaitGroup = nullptr)
    {
        using JobType = Internal::FunctorNode<TFunctor>;
        JobType* job = Memory::DefaultNew<JobType>(std::forward<TFunctor>(functor));
        if (!prerequisites.empty())
            job->AddPrerequisites(prerequisites);

        job->Dispatch(affinity, completionWaitGroup, priority);
    }


    template<class TFunctor>
    void DispatchForeground(const festd::span<WaitGroup* const> prerequisites, TFunctor&& functor,
                            WaitGroup* completionWaitGroup = nullptr)
    {
        Dispatch(FiberAffinityMask::kAll, prerequisites, Priority::kNormal, std::forward<TFunctor>(functor), completionWaitGroup);
    }

    template<class TFunctor>
    void DispatchForeground(const std::initializer_list<const Rc<WaitGroup>> prerequisites, TFunctor&& functor,
                            WaitGroup* completionWaitGroup = nullptr)
    {
        Dispatch(FiberAffinityMask::kAll, prerequisites, Priority::kNormal, std::forward<TFunctor>(functor), completionWaitGroup);
    }


    template<class TFunctor>
    void DispatchForeground(TFunctor&& functor, WaitGroup* completionWaitGroup = nullptr)
    {
        Dispatch(FiberAffinityMask::kAll,
                 festd::span<WaitGroup* const>{},
                 Priority::kNormal,
                 std::forward<TFunctor>(functor),
                 completionWaitGroup);
    }


    template<class TFunctor>
    void DispatchBackground(const festd::span<WaitGroup* const> prerequisites, TFunctor&& functor,
                            WaitGroup* completionWaitGroup = nullptr)
    {
        Dispatch(FiberAffinityMask::kAllBackground,
                 prerequisites,
                 Priority::kNormal,
                 std::forward<TFunctor>(functor),
                 completionWaitGroup);
    }

    template<class TFunctor>
    void DispatchBackground(const std::initializer_list<const Rc<WaitGroup>> prerequisites, TFunctor&& functor,
                            WaitGroup* completionWaitGroup = nullptr)
    {
        Dispatch(FiberAffinityMask::kAllBackground,
                 prerequisites,
                 Priority::kNormal,
                 std::forward<TFunctor>(functor),
                 completionWaitGroup);
    }


    template<class TFunctor>
    void DispatchBackground(TFunctor&& functor, WaitGroup* completionWaitGroup = nullptr)
    {
        Dispatch(FiberAffinityMask::kAllBackground,
                 festd::span<WaitGroup* const>{},
                 Priority::kNormal,
                 std::forward<TFunctor>(functor),
                 completionWaitGroup);
    }


    template<class TFunctor>
    void DispatchMainThread(const festd::span<WaitGroup* const> prerequisites, TFunctor&& functor,
                            WaitGroup* completionWaitGroup = nullptr)
    {
        Dispatch(FiberAffinityMask::kMainThread,
                 prerequisites,
                 Priority::kNormal,
                 std::forward<TFunctor>(functor),
                 completionWaitGroup);
    }

    template<class TFunctor>
    void DispatchMainThread(const std::initializer_list<const Rc<WaitGroup>> prerequisites, TFunctor&& functor,
                            WaitGroup* completionWaitGroup = nullptr)
    {
        Dispatch(FiberAffinityMask::kMainThread,
                 prerequisites,
                 Priority::kNormal,
                 std::forward<TFunctor>(functor),
                 completionWaitGroup);
    }


    template<class TFunctor>
    void DispatchMainThread(TFunctor&& functor, WaitGroup* completionWaitGroup = nullptr)
    {
        Dispatch(FiberAffinityMask::kMainThread,
                 festd::span<WaitGroup* const>{},
                 Priority::kNormal,
                 std::forward<TFunctor>(functor),
                 completionWaitGroup);
    }
} // namespace FE::Jobs
