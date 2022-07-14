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
                                                                     UInt32 imageCount, BufferBarrierDesc* bufferBarriers,
                                                                     UInt32 bufferCount)
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
                                                          UInt32 clearValueCount)
        {
            self->BeginRenderPass(renderPass, framebuffer, ArraySlice(clearValues, clearValues + clearValueCount));
        }

        FE_DLL_EXPORT void ICommandBuffer_EndRenderPass(ICommandBuffer* self)
        {
            self->EndRenderPass();
        }

        FE_DLL_EXPORT void ICommandBuffer_BindVertexBuffers(ICommandBuffer* self, UInt32 startSlot, UInt32 slotCount,
                                                            IBuffer** buffers, UInt64* offsets)
        {
            self->BindVertexBuffers(
                startSlot, ArraySlice(buffers, buffers + slotCount), ArraySlice(offsets, offsets + slotCount));
        }

        FE_DLL_EXPORT void ICommandBuffer_BindVertexBuffer(ICommandBuffer* self, UInt32 slot, IBuffer* buffer, UInt64 byteOffset)
        {
            self->BindVertexBuffer(slot, buffer, byteOffset);
        }

        FE_DLL_EXPORT void ICommandBuffer_BindIndexBuffer(ICommandBuffer* self, IBuffer* buffer, UInt64 byteOffset)
        {
            self->BindIndexBuffer(buffer, byteOffset);
        }

        FE_DLL_EXPORT void ICommandBuffer_BindDescriptorTables(ICommandBuffer* self, IDescriptorTable** descriptorTables,
                                                               UInt32 descriptorTableCount, IGraphicsPipeline* pipeline)
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

        FE_DLL_EXPORT void ICommandBuffer_Draw(ICommandBuffer* self, UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex,
                                               UInt32 firstInstance)
        {
            self->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
        }

        FE_DLL_EXPORT void ICommandBuffer_DrawIndexed(ICommandBuffer* self, UInt32 indexCount, UInt32 instanceCount,
                                                      UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance)
        {
            self->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        }

        FE_DLL_EXPORT void ICommandBuffer_BindGraphicsPipeline(ICommandBuffer* self, IGraphicsPipeline* pipeline)
        {
            self->BindGraphicsPipeline(pipeline);
        }
    }
} // namespace FE::Osmium
