#pragma once
#include <OsGPU/CommandBuffer/ICommandBuffer.h>
#include <OsGPU/Common/VKConfig.h>

namespace FE::Osmium
{
    class VKDevice;

    class VKCommandBuffer : public ICommandBuffer
    {
        VKDevice* m_Device;
        VkCommandBuffer m_CommandBuffer;
        VkCommandPool m_CommandPool;
        bool m_IsUpdating;

    public:
        FE_RTTI_Class(VKCommandBuffer, "9A422C8F-72E8-48DD-8BE6-63D059AE9432");

        VKCommandBuffer(VKDevice& dev, CommandQueueClass cmdQueueClass);
        VKCommandBuffer(VKDevice& dev, uint32_t queueFamilyIndex);
        ~VKCommandBuffer() override;

        VkCommandBuffer GetNativeBuffer();

        void Begin() override;
        void End() override;

        void BindDescriptorTables(const ArraySlice<IDescriptorTable*>& descriptorTables, IGraphicsPipeline* pipeline) override;
        void BindGraphicsPipeline(IGraphicsPipeline* pipeline) override;

        void BeginRenderPass(IRenderPass* renderPass, IFramebuffer* framebuffer,
                             const ArraySlice<ClearValueDesc>& clearValues) override;
        void EndRenderPass() override;

        void BindIndexBuffer(IBuffer* buffer, uint64_t byteOffset) override;
        void BindVertexBuffer(uint32_t slot, IBuffer* buffer, uint64_t byteOffset) override;
        void BindVertexBuffers(uint32_t startSlot, const ArraySlice<IBuffer*>& buffers, const ArraySlice<uint64_t>& offsets) override;

        void CopyBuffers(IBuffer* source, IBuffer* dest, const BufferCopyRegion& region) override;
        void CopyBufferToImage(IBuffer* source, IImage* dest, const BufferImageCopyRegion& region) override;

        void BlitImage(IImage* source, IImage* dest, const ImageBlitRegion& region) override;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                         uint32_t firstInstance) override;

        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const Scissor& scissor) override;

        virtual void ResourceTransitionBarriers(const ArraySlice<ImageBarrierDesc>& imageBarriers,
                                                const ArraySlice<BufferBarrierDesc>& bufferBarriers) override;
        void MemoryBarrier() override;
    };
} // namespace FE::Osmium
