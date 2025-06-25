#pragma once
#include <Graphics/Core/Common/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/CommandBuffer.h>
#include <Graphics/Core/Vulkan/ResourceBarrierBatcher.h>

namespace FE::Graphics::Vulkan
{
    struct BindlessManager;

    struct FrameGraphContext final : public Common::FrameGraphContext
    {
        FE_RTTI_Class(FrameGraphContext, "C27FC437-A09A-49F7-B3B6-DEE56C0CF04F");

        FrameGraphContext(Core::Device* device, Core::FrameGraph* frameGraph, BindlessManager* bindlessManager);

        void Init(CommandBuffer* graphicsCommandBuffer);

        void BeginRendering(VkCommandBuffer vkCommandBuffer) const;

        void DrawImpl(const Core::DrawCall& drawCall) override;
        void DispatchMeshImpl(const Core::GraphicsPipeline* pipeline, Vector3UInt workGroupCount, uint32_t stencilRef) override;
        void DispatchImpl(const Core::ComputePipeline* pipeline, Vector3UInt workGroupCount) override;

        void EnqueueFenceToSignal(const Core::FenceSyncPoint& fence) override;
        void EnqueueFenceToWait(const Core::FenceSyncPoint& fence) override;

        struct WaitSemaphore final
        {
            Rc<Semaphore> m_semaphore;
            VkPipelineStageFlags m_stageMask;
        };

        BindlessManager* m_bindlessManager;

        ResourceBarrierBatcher m_resourceBarrierBatcher;
        SegmentedVector<Rc<Semaphore>, 256> m_signalSemaphores;
        SegmentedVector<WaitSemaphore, 512> m_waitSemaphores;

        Rc<CommandBuffer> m_graphicsCommandBuffer;
    };

    FE_ENABLE_IMPL_CAST(FrameGraphContext);
} // namespace FE::Graphics::Vulkan
