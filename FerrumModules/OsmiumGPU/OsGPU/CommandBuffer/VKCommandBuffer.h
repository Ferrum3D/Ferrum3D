﻿#pragma once
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
        FE_CLASS_RTTI(VKCommandBuffer, "9A422C8F-72E8-48DD-8BE6-63D059AE9432");

        VKCommandBuffer(VKDevice& dev, CommandQueueClass cmdQueueClass);
        VKCommandBuffer(VKDevice& dev, UInt32 queueFamilyIndex);
        ~VKCommandBuffer() override;

        VkCommandBuffer GetNativeBuffer();

        void Begin() override;
        void End() override;

        void BindDescriptorTables(const ArraySlice<IDescriptorTable*>& descriptorTables, IGraphicsPipeline* pipeline) override;
        void BindGraphicsPipeline(IGraphicsPipeline* pipeline) override;

        void BeginRenderPass(IRenderPass* renderPass, IFramebuffer* framebuffer,
                             const ArraySlice<ClearValueDesc>& clearValues) override;
        void EndRenderPass() override;

        void BindIndexBuffer(IBuffer* buffer, UInt64 byteOffset) override;
        void BindVertexBuffer(UInt32 slot, IBuffer* buffer, UInt64 byteOffset) override;
        void BindVertexBuffers(UInt32 startSlot, const ArraySlice<IBuffer*>& buffers, const ArraySlice<UInt64>& offsets) override;

        void CopyBuffers(IBuffer* source, IBuffer* dest, const BufferCopyRegion& region) override;
        void CopyBufferToImage(IBuffer* source, IImage* dest, const BufferImageCopyRegion& region) override;

        void BlitImage(IImage* source, IImage* dest, const ImageBlitRegion& region) override;

        void Draw(UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance) override;
        void DrawIndexed(UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset,
                         UInt32 firstInstance) override;

        void SetViewport(const Viewport& viewport) override;
        void SetScissor(const Scissor& scissor) override;

        virtual void ResourceTransitionBarriers(const ArraySlice<ImageBarrierDesc>& imageBarriers,
                                                const ArraySlice<BufferBarrierDesc>& bufferBarriers) override;
        void MemoryBarrier() override;
    };
} // namespace FE::Osmium
