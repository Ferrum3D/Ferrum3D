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

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_BindVertexBuffers")]
        private static extern void BindVertexBuffersNative(IntPtr self, uint startSlot, uint slotCount, IntPtr buffers,
            IntPtr offsets);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_BindVertexBuffer")]
        private static extern void BindVertexBufferNative(IntPtr self, uint slot, IntPtr buffer, ulong byteOffset);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_BindIndexBuffer")]
        private static extern void BindIndexBufferNative(IntPtr self, IntPtr buffer, ulong byteOffset);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_BindDescriptorTables")]
        private static extern void BindDescriptorTablesNative(IntPtr self, IntPtr[] descriptorTables, uint count,
            IntPtr pipeline);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_CopyBuffers")]
        private static extern void CopyBuffersNative(IntPtr self, IntPtr source, IntPtr dest,
            ref BufferCopyRegion region);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_ResourceTransitionBarriers")]
        private static extern void ResourceTransitionBarriersNative(IntPtr self, IntPtr imageBarriers, uint imageCount,
            IntPtr bufferBarriers, uint bufferCount);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_CopyBufferToImage")]
        private static extern void CopyBufferToImageNative(IntPtr self, IntPtr source, IntPtr dest,
            ref BufferImageCopyRegion region);

        [DllImport("OsGPUBindings", EntryPoint = "ICommandBuffer_BlitImage")]
        private static extern void BlitImageNative(IntPtr self, IntPtr source, IntPtr dest,
            ref ImageBlitRegion region);

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

            public unsafe void BindVertexBuffers(uint startSlot, uint slotCount, IntPtr* bufferHandles, ulong* offsets)
            {
                BindVertexBuffersNative(handle, startSlot, slotCount, new IntPtr(bufferHandles), new IntPtr(offsets));
            }

            public void BindVertexBuffer(uint slot, Buffer vertexBuffer, ulong byteOffset = 0)
            {
                BindVertexBufferNative(handle, slot, vertexBuffer.Handle, byteOffset);
            }

            public void BindIndexBuffer(Buffer indexBuffer, ulong byteOffset = 0)
            {
                BindIndexBufferNative(handle, indexBuffer.Handle, byteOffset);
            }

            public void BindDescriptorTables(GraphicsPipeline pipeline, params DescriptorTable[] descriptorTables)
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

            public void BlitImage(Image source, Image dest, ImageBlitRegion region)
            {
                BlitImageNative(handle, source.Handle, dest.Handle, ref region);
            }

            public void ResourceTransitionBarriers(ImageBarrierDesc[] imageBarriers, BufferBarrierDesc[] bufferBarriers)
            {
                unsafe
                {
                    fixed (ImageBarrierDesc* ip = imageBarriers)
                    fixed (BufferBarrierDesc* bp = bufferBarriers)
                    {
                        ResourceTransitionBarriersNative(handle, new IntPtr(ip), (uint)imageBarriers.Length,
                            new IntPtr(bp), (uint)bufferBarriers.Length);
                    }
                }
            }

            public void TransitionImageState(in ImageBarrierDesc barrier)
            {
                unsafe
                {
                    fixed (ImageBarrierDesc* p = &barrier)
                    {
                        ResourceTransitionBarriersNative(handle, new IntPtr(p), 1u, IntPtr.Zero, 0);
                    }
                }
            }

            public void TransitionBufferState(in BufferBarrierDesc barrier)
            {
                unsafe
                {
                    fixed (BufferBarrierDesc* p = &barrier)
                    {
                        ResourceTransitionBarriersNative(handle, IntPtr.Zero, 0, new IntPtr(p), 1u);
                    }
                }
            }

            public void TransitionBufferState(Buffer buffer, ResourceState stateBefore, ResourceState stateAfter)
            {
                TransitionBufferState(new BufferBarrierDesc(buffer, 0, buffer.Size, stateBefore, stateAfter));
            }

            public void TransitionImageState(Image image, ResourceState stateAfter,
                ImageAspectFlags aspectFlags = ImageAspectFlags.Color)
            {
                var subresourceRange =
                    new ImageSubresourceRange(0, image.MipSliceCount, 0, image.ArraySize, aspectFlags);
                TransitionImageState(new ImageBarrierDesc(image, subresourceRange, stateAfter));
            }

            public void TransitionImageState(Image image, ResourceState stateAfter, int mipSlice,
                int mipSliceCount = 1)
            {
                TransitionImageState(
                    new ImageBarrierDesc(image, stateAfter, mipSlice, mipSliceCount));
            }

            public void Draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance)
            {
                DrawNative(handle, vertexCount, instanceCount, firstVertex, firstInstance);
            }

            public void DrawIndexed(uint indexCount, uint instanceCount, uint firstIndex, int vertexOffset,
                uint firstInstance)
            {
                DrawIndexedNative(handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
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
