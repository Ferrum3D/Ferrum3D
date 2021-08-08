#pragma once
#include <FeGPU/CommandBuffer/ICommandBuffer.h>
#include <FeGPU/Device/IDevice.h>
#include <FeGPU/Common/VKConfig.h>

namespace FE::GPU
{
    class VKDevice;

    class VKCommandBuffer : public Object<ICommandBuffer>
    {
        VKDevice* m_Device;
        vk::UniqueCommandBuffer m_CommandBuffer;
        bool m_IsUpdating;

    public:
        VKCommandBuffer(VKDevice& dev, CommandQueueClass cmdQueueClass);

        vk::CommandBuffer& GetNativeBuffer();

        virtual void Begin() override;
        virtual void End() override;

        virtual void Draw(UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance) override;
        virtual void DrawIndexed(UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance) override;

        virtual void SetViewport(const Viewport& viewport) override;
        virtual void SetScissor(const Scissor& scissor) override;

        virtual void ResourceTransitionBarriers(const Vector<ResourceTransitionBarrierDesc>& barriers) override;
        virtual void MemoryBarrier() override;
    };
}
