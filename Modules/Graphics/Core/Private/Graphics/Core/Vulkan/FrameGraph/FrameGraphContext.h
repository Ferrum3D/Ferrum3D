#pragma once
#include <Graphics/Core/Common/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/CommandBuffer.h>

namespace FE::Graphics::Vulkan
{
    struct DescriptorManager;

    struct FrameGraphContext final : public Common::FrameGraphContext
    {
        FE_RTTI("C27FC437-A09A-49F7-B3B6-DEE56C0CF04F");

        FrameGraphContext(Core::Device* device, Core::FrameGraph* frameGraph, Core::DescriptorManager* descriptorManager);

        void Init(CommandBuffer* graphicsCommandBuffer);

        void BeginRendering(VkCommandBuffer vkCommandBuffer) const;

        void DrawImpl(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset,
                      uint32_t instanceOffset) override;
        void DispatchMeshImpl(Vector3UInt workGroupCount) override;
        void DispatchImpl(Vector3UInt workGroupCount) override;

        void EnqueueFenceToSignal(const Core::FenceSyncPoint& fence) override;
        void EnqueueFenceToWait(const Core::FenceSyncPoint& fence) override;

    private:
        DescriptorManager* m_descriptorManager;
        Rc<CommandBuffer> m_graphicsCommandBuffer;
    };

    FE_ENABLE_IMPL_CAST(FrameGraphContext);
} // namespace FE::Graphics::Vulkan
