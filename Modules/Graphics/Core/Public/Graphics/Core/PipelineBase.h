#pragma once
#include <Core/Jobs/WaitGroup.h>
#include <Graphics/Core/Device.h>

namespace FE::Graphics::Core
{
    enum class PipelineStatus : uint32_t
    {
        kNotReady,
        kReady,
        kError,
    };


    struct PipelineBase : public Memory::RefCountedObjectBase
    {
        FE_RTTI("8D4EC84B-525C-4A21-9FBD-1C304F3471D2");

        [[nodiscard]] PipelineStatus GetStatus() const
        {
            return m_status.load(std::memory_order_acquire);
        }

        [[nodiscard]] bool IsReady() const
        {
            return m_status.load(std::memory_order_acquire) == PipelineStatus::kReady;
        }

        //! Size of the push constants the shaders declare; zero when they declare none or the pipeline is not ready.
        [[nodiscard]] uint32_t GetRootConstantsByteSize() const
        {
            return m_rootConstantsByteSize;
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
        Device* m_device = nullptr;
        std::atomic<PipelineStatus> m_status = PipelineStatus::kNotReady;
        Rc<WaitGroup> m_completionWaitGroup;
        uint32_t m_rootConstantsByteSize = 0;
    };
} // namespace FE::Graphics::Core
