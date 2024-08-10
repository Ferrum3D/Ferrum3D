#include <OsGPU/CommandBuffer/ICommandBuffer.h>
#include <OsGPU/Descriptors/IDescriptorTable.h>

namespace FE::Osmium
{
    extern "C"
    {
        FE_DLL_EXPORT void ICommandBuffer_Destruct(ICommandBuffer* self)
        {
            self->ReleaseStrongRef();
        }

        FE_DLL_EXPORT void ICommandBuffer_Begin(ICommandBuffer* self)
        {
            self->Begin();
        }

        FE_DLL_EXPORT void ICommandBuffer_End(ICommandBuffer* self)
        {
            self->End();
        }

        FE_DLL_EXPORT void ICommandBuffer_SetViewport(ICommandBuffer* self, Viewport* viewport)
        {
            self->SetViewport(*viewport);
        }

        FE_DLL_EXPORT void ICommandBuffer_SetScissor(ICommandBuffer* self, Scissor* rect)
        {
            self->SetScissor(*rect);
        }

        FE_DLL_EXPORT void ICommandBuffer_ResourceTransitionBarriers(ICommandBuffer* self, ImageBarrierDesc* imageBarriers,
                                                                     uint32_t imageCount, BufferBarrierDesc* bufferBarriers,
                                                                     uint32_t bufferCount)
        {
            self->ResourceTransitionBarriers(ArraySlice(imageBarriers, imageBarriers + imageCount),
                                             ArraySlice(bufferBarriers, bufferBarriers + bufferCount));
        }

        FE_DLL_EXPORT void ICommandBuffer_MemoryBarrier(ICommandBuffer* self)
        {
            self->MemoryBarrier();
        }

        FE_DLL_EXPORT void ICommandBuffer_BeginRenderPass(ICommandBuffer* self, IRenderPass* renderPass,
                                                          IFramebuffer* framebuffer, ClearValueDesc* clearValues,
                                                          uint32_t clearValueCount)
        {
            self->BeginRenderPass(renderPass, framebuffer, ArraySlice(clearValues, clearValues + clearValueCount));
        }

        FE_DLL_EXPORT void ICommandBuffer_EndRenderPass(ICommandBuffer* self)
        {
            self->EndRenderPass();
        }

        FE_DLL_EXPORT void ICommandBuffer_BindVertexBuffers(ICommandBuffer* self, uint32_t startSlot, uint32_t slotCount,
                                                            IBuffer** buffers, uint64_t* offsets)
        {
            self->BindVertexBuffers(
                startSlot, ArraySlice(buffers, buffers + slotCount), ArraySlice(offsets, offsets + slotCount));
        }

        FE_DLL_EXPORT void ICommandBuffer_BindVertexBuffer(ICommandBuffer* self, uint32_t slot, IBuffer* buffer, uint64_t byteOffset)
        {
            self->BindVertexBuffer(slot, buffer, byteOffset);
        }

        FE_DLL_EXPORT void ICommandBuffer_BindIndexBuffer(ICommandBuffer* self, IBuffer* buffer, uint64_t byteOffset)
        {
            self->BindIndexBuffer(buffer, byteOffset);
        }

        FE_DLL_EXPORT void ICommandBuffer_BindDescriptorTables(ICommandBuffer* self, IDescriptorTable** descriptorTables,
                                                               uint32_t descriptorTableCount, IGraphicsPipeline* pipeline)
        {
            self->BindDescriptorTables(ArraySlice(descriptorTables, descriptorTables + descriptorTableCount), pipeline);
        }

        FE_DLL_EXPORT void ICommandBuffer_CopyBuffers(ICommandBuffer* self, IBuffer* source, IBuffer* dest,
                                                      BufferCopyRegion* region)
        {
            self->CopyBuffers(source, dest, *region);
        }

        FE_DLL_EXPORT void ICommandBuffer_CopyBufferToImage(ICommandBuffer* self, IBuffer* source, IImage* dest,
                                                            BufferImageCopyRegion* region)
        {
            self->CopyBufferToImage(source, dest, *region);
        }

        FE_DLL_EXPORT void ICommandBuffer_BlitImage(ICommandBuffer* self, IImage* source, IImage* dest, ImageBlitRegion* region)
        {
            self->BlitImage(source, dest, *region);
        }

        FE_DLL_EXPORT void ICommandBuffer_Draw(ICommandBuffer* self, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                                               uint32_t firstInstance)
        {
            self->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
        }

        FE_DLL_EXPORT void ICommandBuffer_DrawIndexed(ICommandBuffer* self, uint32_t indexCount, uint32_t instanceCount,
                                                      uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
        {
            self->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        }

        FE_DLL_EXPORT void ICommandBuffer_BindGraphicsPipeline(ICommandBuffer* self, IGraphicsPipeline* pipeline)
        {
            self->BindGraphicsPipeline(pipeline);
        }
    }
} // namespace FE::Osmium
