#pragma once
#include <Core/Containers/ConcurrentQueue.h>
#include <Core/Jobs/WaitGroup.h>
#include <Core/Memory/RefCount.h>

namespace FE::Jobs
{
    struct JobNode : public ConcurrentQueue::Node
    {
        JobNode() = default;
        virtual ~JobNode();

        JobNode(const JobNode&) = delete;
        JobNode& operator=(const JobNode&) = delete;
        JobNode(JobNode&&) = delete;
        JobNode& operator=(JobNode&&) = delete;

        virtual void Execute() = 0;

        void AddPrerequisite(WaitGroup* waitGroup);
        void AddPrerequisite(const Rc<WaitGroup>& waitGroup)
        {
            AddPrerequisite(waitGroup.Get());
        }

        void AddPrerequisites(festd::span<WaitGroup* const> waitGroups);
        void AddPrerequisites(festd::span<const Rc<WaitGroup>> waitGroups);

        void AddPrerequisites(const std::initializer_list<WaitGroup*> waitGroups)
        {
            AddPrerequisites(festd::span(waitGroups));
        }

        void AddPrerequisites(const std::initializer_list<const Rc<WaitGroup>> waitGroups)
        {
            AddPrerequisites(festd::span(waitGroups));
        }

        void Dispatch(FiberAffinityMask affinityMask, WaitGroup* completionWaitGroup = nullptr,
                      Priority priority = Priority::kNormal);

        void DispatchForeground(WaitGroup* completionWaitGroup = nullptr, const Priority priority = Priority::kNormal)
        {
            Dispatch(FiberAffinityMask::kAll, completionWaitGroup, priority);
        }

        void DispatchBackground(WaitGroup* completionWaitGroup = nullptr, const Priority priority = Priority::kNormal)
        {
            Dispatch(FiberAffinityMask::kAllBackground, completionWaitGroup, priority);
        }

        [[nodiscard]] WaitGroup* GetCompletionWaitGroup() const
        {
            return m_completionWaitGroup.Get();
        }

    private:
        friend struct JobSystem;
        friend struct FE::WaitGroup;

        bool DependencySatisfied()
        {
            const uint32_t previousValue = m_dependencyCounter.fetch_sub(1, std::memory_order_acq_rel);
            FE_Assert(previousValue > 0, "Job dependency counter underflow");
            return previousValue == 1;
        }

        Rc<WaitGroup> m_completionWaitGroup;
        std::atomic<uint32_t> m_dependencyCounter = 1;
        std::atomic<bool> m_dispatchRequested = false;
        Priority m_priority = Priority::kNormal;
        FiberAffinityMask m_affinityMask = FiberAffinityMask::kNone;
        uint64_t m_orderHint = 0;
    };
} // namespace FE::Jobs
