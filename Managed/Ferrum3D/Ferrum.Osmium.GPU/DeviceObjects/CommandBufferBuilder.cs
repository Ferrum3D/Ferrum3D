using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Math;
using Ferrum.Osmium.GPU.Common;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public struct ClearValueDesc
    {
        public readonly Color Color;

        public ClearValueDesc(Color color)
        {
            Color = color;
        }
    }

    public partial class CommandBuffer
    {
        [DllImport("OsmiumBindings", EntryPoint = "ICommandBuffer_Begin")]
        private static extern IntPtr BeginNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "ICommandBuffer_End")]
        private static extern IntPtr EndNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "ICommandBuffer_SetViewport")]
        private static extern void SetViewportNative(IntPtr self, ref Viewport viewport);

        [DllImport("OsmiumBindings", EntryPoint = "ICommandBuffer_SetScissor")]
        private static extern void SetScissorNative(IntPtr self, ref Scissor scissor);

        [DllImport("OsmiumBindings", EntryPoint = "ICommandBuffer_MemoryBarrier")]
        private static extern void MemoryBarrierNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "ICommandBuffer_BeginRenderPass")]
        private static extern void BeginRenderPassNative(IntPtr self, IntPtr renderPass, IntPtr framebuffer,
            ref ClearValueDesc clearValueDesc);

        [DllImport("OsmiumBindings", EntryPoint = "ICommandBuffer_EndRenderPass")]
        private static extern void EndRenderPassNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "ICommandBuffer_BindVertexBuffer")]
        private static extern void BindVertexBufferNative(IntPtr self, uint slot, IntPtr buffer);

        [DllImport("OsmiumBindings", EntryPoint = "ICommandBuffer_BindIndexBuffer")]
        private static extern void BindIndexBufferNative(IntPtr self, IntPtr buffer);

        [DllImport("OsmiumBindings", EntryPoint = "ICommandBuffer_Draw")]
        private static extern void DrawNative(IntPtr self, uint vertexCount, uint instanceCount, uint firstVertex,
            uint firstInstance);

        [DllImport("OsmiumBindings", EntryPoint = "ICommandBuffer_BindGraphicsPipeline")]
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

            public void BeginRenderPass(RenderPass renderPass, Framebuffer framebuffer, Color clearColor)
            {
                var clearValueDesc = new ClearValueDesc(clearColor);
                BeginRenderPassNative(handle, renderPass.Handle, framebuffer.Handle, ref clearValueDesc);
            }

            public void BeginRenderPass(RenderPass renderPass, Framebuffer framebuffer, ClearValueDesc clearValueDesc)
            {
                BeginRenderPassNative(handle, renderPass.Handle, framebuffer.Handle, ref clearValueDesc);
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

            public void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance)
            {
                DrawNative(handle, (uint)vertexCount, (uint)instanceCount, (uint)firstVertex, (uint)firstInstance);
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
