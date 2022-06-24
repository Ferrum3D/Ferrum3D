#include <GPU/CommandBuffer/ICommandBuffer.h>
#include <GPU/Descriptors/IDescriptorTable.h>

namespace FE::GPU
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

        FE_DLL_EXPORT void ICommandBuffer_MemoryBarrier(ICommandBuffer* self)
        {
            self->MemoryBarrier();
        }

        FE_DLL_EXPORT void ICommandBuffer_BeginRenderPass(
            ICommandBuffer* self, IRenderPass* renderPass, IFramebuffer* framebuffer, ClearValueDesc* clearValue)
        {
            ClearValueDesc c = *clearValue;
            self->BeginRenderPass(renderPass, framebuffer, c);
        }

        FE_DLL_EXPORT void ICommandBuffer_EndRenderPass(ICommandBuffer* self)
        {
            self->EndRenderPass();
        }

        FE_DLL_EXPORT void ICommandBuffer_BindVertexBuffer(ICommandBuffer* self, UInt32 slot, IBuffer* buffer)
        {
            self->BindVertexBuffer(slot, buffer);
        }

        FE_DLL_EXPORT void ICommandBuffer_BindIndexBuffer(ICommandBuffer* self, IBuffer* buffer)
        {
            self->BindIndexBuffer(buffer);
        }

        FE_DLL_EXPORT void ICommandBuffer_BindDescriptorTables(
            ICommandBuffer* self, IDescriptorTable** descriptorTables, UInt32 descriptorTableCount, IGraphicsPipeline* pipeline)
        {
            List<IDescriptorTable*> d;
            d.Assign(descriptorTables, descriptorTables + descriptorTableCount);
            self->BindDescriptorTables(d, pipeline);
        }

        FE_DLL_EXPORT void ICommandBuffer_CopyBuffers(
            ICommandBuffer* self, IBuffer* source, IBuffer* dest, BufferCopyRegion* region)
        {
            self->CopyBuffers(source, dest, *region);
        }

        FE_DLL_EXPORT void ICommandBuffer_Draw(
            ICommandBuffer* self, UInt32 vertexCount, UInt32 instanceCount, UInt32 firstVertex, UInt32 firstInstance)
        {
            self->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
        }

        FE_DLL_EXPORT void ICommandBuffer_DrawIndexed(
            ICommandBuffer* self, UInt32 indexCount, UInt32 instanceCount, UInt32 firstIndex, Int32 vertexOffset, UInt32 firstInstance)
        {
            self->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        }

        FE_DLL_EXPORT void ICommandBuffer_BindGraphicsPipeline(ICommandBuffer* self, IGraphicsPipeline* pipeline)
        {
            self->BindGraphicsPipeline(pipeline);
        }
    }
} // namespace FE::GPU
