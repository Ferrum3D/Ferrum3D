#pragma once
#include <OsGPU/CommandBuffer/ICommandBuffer.h>
#include <OsGPU/Common/VKConfig.h>

namespace FE::GPU
{
    class VKDevice;

    class VKCommandBuffer : public Object<ICommandBuffer>
    {
        VKDevice* m_Device;
        vk::UniqueCommandBuffer m_CommandBuffer;
        bool m_IsUpdating;

    public:
        FE_CLASS_RTTI(VKCommandBuffer, "9A422C8F-72E8-48DD-8BE6-63D059AE9432");

        VKCommandBuffer(VKDevice& dev, CommandQueueClass cmdQueueClass);
        VKCommandBuffer(VKDevice& dev, UInt32 queueFamilyIndex);

        vk::CommandBuffer& GetNativeBuffer();

        void Begin() override;
        void End() override;

        void BindDescriptorTables(const List<IDescriptorTable*>& descriptorTables, IGraphicsPipeline* pipeline) override;
        void BindGraphicsPipeline(IGraphicsPipeline* pipeline) override;

        void BeginRenderPass(IRenderPass* renderPass, IFramebuffer* framebuffer, const ClearValueDesc& clearValue) override;
        void EndRenderPass() override;

        void BindVertexBuffer(UInt32 slot, IBuffer* buffer) override;
        void BindIndexBuffer(IBuffer* buffer) override;

        void CopyBuffers(IBuffer* source, IBuffer* dest, const BufferCopyRegion& region) override;

        void Draw(UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance) override;
        void DrawIndexed(
            UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance) override;

        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const Scissor& scissor) override;

        void ResourceTransitionBarriers(const List<ResourceTransitionBarrierDesc>& barriers) override;
        void MemoryBarrier() override;
    };
} // namespace FE::GPU
