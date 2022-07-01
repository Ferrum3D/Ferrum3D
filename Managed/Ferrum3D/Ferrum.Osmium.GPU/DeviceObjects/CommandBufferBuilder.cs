using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Math;
using Ferrum.Osmium.GPU.Common;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public partial class CommandBuffer
    {
        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_Begin")]
        private static extern IntPtr BeginNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_End")]
        private static extern IntPtr EndNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_SetViewport")]
        private static extern void SetViewportNative(IntPtr self, ref Viewport viewport);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_SetScissor")]
        private static extern void SetScissorNative(IntPtr self, ref Scissor scissor);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_MemoryBarrier")]
        private static extern void MemoryBarrierNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_BeginRenderPass")]
        private static extern void BeginRenderPassNative(IntPtr self, IntPtr renderPass, IntPtr framebuffer,
            IntPtr clearValues, uint clearValueCount);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_EndRenderPass")]
        private static extern void EndRenderPassNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_BindVertexBuffer")]
        private static extern void BindVertexBufferNative(IntPtr self, uint slot, IntPtr buffer);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_BindIndexBuffer")]
        private static extern void BindIndexBufferNative(IntPtr self, IntPtr buffer);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_BindDescriptorTables")]
        private static extern void BindDescriptorTablesNative(IntPtr self, IntPtr[] descriptorTables, uint count,
            IntPtr pipeline);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_CopyBuffers")]
        private static extern void CopyBuffersNative(IntPtr self, IntPtr source, IntPtr dest,
            ref BufferCopyRegion region);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_ResourceTransitionBarriers")]
        private static extern void ResourceTransitionBarriersNative(IntPtr self, IntPtr barriers, uint count);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_CopyBufferToImage")]
        private static extern void CopyBufferToImageNative(IntPtr self, IntPtr source, IntPtr dest,
            ref BufferImageCopyRegion region);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_Draw")]
        private static extern void DrawNative(IntPtr self, uint vertexCount, uint instanceCount, uint firstVertex,
            uint firstInstance);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_DrawIndexed")]
        private static extern void DrawIndexedNative(IntPtr self, uint indexCount, uint instanceCount, uint firstIndex,
            int vertexOffset, uint firstInstance);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_BindGraphicsPipeline")]
        private static extern void BindGraphicsPipelineNative(IntPtr self, IntPtr pipeline);

        public class Builder : IDisposable
        {
            private readonly IntPtr handle;

            internal Builder(CommandBuffer commandBuffer)
            {
                handle = commandBuffer.Handle;
                BeginNative(handle);
            }

            public void SetViewport(Viewport viewport)
            {
                SetViewportNative(handle, ref viewport);
            }

            public void SetScissor(Scissor scissor)
            {
                SetScissorNative(handle, ref scissor);
            }

            public void MemoryBarrier()
            {
                MemoryBarrierNative(handle);
            }

            public void BeginRenderPass(RenderPass renderPass, Framebuffer framebuffer,
                params ClearValueDesc[] clearValues)
            {
                unsafe
                {
                    fixed (ClearValueDesc* ptr = clearValues)
                    {
                        BeginRenderPassNative(handle, renderPass.Handle, framebuffer.Handle, new IntPtr(ptr),
                            (uint)clearValues.Length);
                    }
                }
            }

            public void BeginRenderPass(RenderPass renderPass, Framebuffer framebuffer, Color clearColor)
            {
                BeginRenderPass(renderPass, framebuffer, ClearValueDesc.CreateColorValue(clearColor));
            }

            public void EndRenderPass()
            {
                EndRenderPassNative(handle);
            }

            public void BindVertexBuffer(int slot, Buffer vertexBuffer)
            {
                BindVertexBufferNative(handle, (uint)slot, vertexBuffer.Handle);
            }

            public void BindIndexBuffer(Buffer indexBuffer)
            {
                BindIndexBufferNative(handle, indexBuffer.Handle);
            }

            public void BindDescriptorTables(DescriptorTable[] descriptorTables, GraphicsPipeline pipeline)
            {
                var handles = new IntPtr[descriptorTables.Length];
                for (var i = 0; i < descriptorTables.Length; i++)
                {
                    handles[i] = descriptorTables[i].Handle;
                }

                BindDescriptorTablesNative(handle, handles, (uint)descriptorTables.Length, pipeline.Handle);
            }

            public void CopyBuffers(Buffer source, Buffer dest, ulong size)
            {
                CopyBuffers(source, dest, new BufferCopyRegion(size));
            }

            public void CopyBuffers(Buffer source, Buffer dest, BufferCopyRegion region)
            {
                CopyBuffersNative(handle, source.Handle, dest.Handle, ref region);
            }

            public void CopyBufferToImage(Buffer source, Image dest, BufferImageCopyRegion region)
            {
                CopyBufferToImageNative(handle, source.Handle, dest.Handle, ref region);
            }

            public void CopyBufferToImage(Buffer source, Image dest, Size size)
            {
                CopyBufferToImage(source, dest, new BufferImageCopyRegion(size));
            }

            public void ResourceTransitionBarriers(ResourceTransitionBarrierDesc[] barriers)
            {
                var nativeBarriers = new ResourceTransitionBarrierDesc.Native[barriers.Length];
                for (var i = 0; i < barriers.Length; ++i)
                {
                    nativeBarriers[i] = new ResourceTransitionBarrierDesc.Native(barriers[i]);
                }

                unsafe
                {
                    fixed (ResourceTransitionBarrierDesc.Native* ptr = nativeBarriers)
                    {
                        ResourceTransitionBarriersNative(handle, new IntPtr(ptr), (uint)nativeBarriers.Length);
                    }
                }
            }

            public void ResourceTransitionBarrier(ResourceTransitionBarrierDesc barrier)
            {
                var nativeBarrier = new ResourceTransitionBarrierDesc.Native(barrier);
                unsafe
                {
                    ResourceTransitionBarriersNative(handle, new IntPtr(&nativeBarrier), 1u);
                }
            }

            public void ResourceTransitionBarrier(Image image, ResourceState stateAfter)
            {
                ResourceTransitionBarrier(new ResourceTransitionBarrierDesc(image, stateAfter));
            }

            public void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance)
            {
                DrawNative(handle, (uint)vertexCount, (uint)instanceCount, (uint)firstVertex, (uint)firstInstance);
            }

            public void DrawIndexed(int indexCount, int instanceCount, int firstIndex, int vertexOffset,
                int firstInstance)
            {
                DrawIndexedNative(handle, (uint)indexCount, (uint)instanceCount, (uint)firstIndex, vertexOffset,
                    (uint)firstInstance);
            }

            public void BindGraphicsPipeline(GraphicsPipeline pipeline)
            {
                BindGraphicsPipelineNative(handle, pipeline.Handle);
            }

            public void Dispose()
            {
                EndNative(handle);
            }
        }
    }
}
