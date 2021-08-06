#pragma once
#include <FeGPU/CommandBuffer/ICommandBuffer.h>
#include <FeGPU/Device/IDevice.h>
#include <FeGPU/Common/VKConfig.h>

namespace FE::GPU
{
    class VKDevice;

    class VKCommandBuffer : public ICommandBuffer
    {
        VKDevice* m_Device;
        vk::UniqueCommandBuffer m_CommandBuffer;
        bool m_IsUpdating;

    public:
        VKCommandBuffer(VKDevice& dev, CommandQueueClass cmdQueueClass);

        vk::CommandBuffer& GetNativeBuffer();

        virtual void Begin() override;
        virtual void End() override;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;

        virtual void SetViewport(const Viewport& viewport) override;
        virtual void SetScissor(const Scissor& scissor) override;

        virtual void ResourceTransitionBarriers(const Vector<ResourceTransitionBarrierDesc>& barriers) override;
        virtual void MemoryBarrier() override;
    };
}
