#pragma once
#include <Graphics/Core/Common/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/ResourceBarrierBatcher.h>

namespace FE::Graphics::Vulkan
{
    struct FrameGraphContext final : public Common::FrameGraphContext
    {
        FE_RTTI_Class(FrameGraphContext, "C27FC437-A09A-49F7-B3B6-DEE56C0CF04F");

        FrameGraphContext(Core::Device* device, Core::FrameGraph* frameGraph);

        void Init(CommandBuffer* graphicsCommandBuffer);
        void Submit();

        void DrawImpl(const Core::DrawList& drawList) override;

        void EnqueueSemaphoreToWait(Semaphore* semaphore, const VkPipelineStageFlags stageMask)
        {
            FE_Assert(semaphore && semaphore->GetNative());
            m_waitSemaphores.push_back({ semaphore, stageMask });
        }

        void EnqueueSemaphoreToSignal(Semaphore* semaphore)
        {
            FE_Assert(semaphore && semaphore->GetNative());
            m_signalSemaphores.push_back(semaphore);
        }

        struct WaitSemaphore final
        {
            Rc<Semaphore> m_semaphore;
            VkPipelineStageFlags m_stageMask;
        };

        ResourceBarrierBatcher m_resourceBarrierBatcher;
        SegmentedVector<Rc<Semaphore>, 256> m_signalSemaphores;
        SegmentedVector<WaitSemaphore, 512> m_waitSemaphores;

        Rc<CommandBuffer> m_graphicsCommandBuffer;
    };

    FE_ENABLE_IMPL_CAST(FrameGraphContext);
} // namespace FE::Graphics::Vulkan
