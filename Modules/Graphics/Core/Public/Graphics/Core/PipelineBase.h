#pragma once
#include <FeCore/Jobs/WaitGroup.h>
#include <Graphics/Core/DeviceObject.h>

namespace FE::Graphics::Core
{
    enum class PipelineStatus : uint32_t
    {
        kNotReady,
        kReady,
        kError,
    };


    struct PipelineBase : public DeviceObject
    {
        FE_RTTI_Class(PipelineBase, "8D4EC84B-525C-4A21-9FBD-1C304F3471D2");

        [[nodiscard]] PipelineStatus GetStatus() const
        {
            return m_status.load(std::memory_order_acquire);
        }

        [[nodiscard]] bool IsReady() const
        {
            return m_status.load(std::memory_order_acquire) == PipelineStatus::kReady;
        }

        [[nodiscard]] WaitGroup* GetCompletionWaitGroup() const
        {
            return m_completionWaitGroup.Get();
        }

        void SetCompletionWaitGroup(WaitGroup* waitGroup)
        {
            FE_Assert(m_completionWaitGroup == nullptr, "Already set");
            m_completionWaitGroup = waitGroup;
        }

    protected:
        std::atomic<PipelineStatus> m_status = PipelineStatus::kNotReady;
        Rc<WaitGroup> m_completionWaitGroup;
    };
} // namespace FE::Graphics::Core
