using System;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.CommandLists
{
    public class CommandList : IDisposable
    {
        internal CommandBuffer.Builder NativeBuilder { get; }

        private CommandList(CommandBuffer.Builder builder)
        {
            NativeBuilder = builder;
        }

        internal static CommandList Begin(CommandBuffer commandBuffer)
        {
            return new CommandList(commandBuffer.Begin());
        }

        public void Submit(in DrawItem drawItem)
        {
            drawItem.PipelineState.Bind(NativeBuilder);
            // Bind shaders resources...

            NativeBuilder.SetScissor(drawItem.Scissor);
            NativeBuilder.SetViewport(drawItem.Viewport);

            NativeBuilder.BindIndexBuffer(drawItem.Indices.Buffer);
            unsafe
            {
                var vertexCount = drawItem.Vertices.Count;

                var buffers = stackalloc IntPtr[vertexCount];
                for (var i = 0; i < drawItem.Vertices.Count; i++)
                {
                    buffers[i] = drawItem.Vertices[i].Buffer.Handle;
                }

                var offsets = stackalloc ulong[vertexCount];
                for (var i = 0; i < drawItem.Vertices.Count; i++)
                {
                    offsets[i] = drawItem.Vertices[i].ByteOffset;
                }

                NativeBuilder.BindVertexBuffers(0, (uint)vertexCount, buffers, offsets);
            }

            switch (drawItem.Arguments.DrawType)
            {
                case DrawType.Linear:
                    NativeBuilder.Draw(drawItem.Arguments.ElementCount, drawItem.Arguments.InstanceCount,
                        drawItem.Arguments.FirstElement, drawItem.Arguments.FirstInstance);
                    break;
                case DrawType.Indexed:
                    NativeBuilder.DrawIndexed(drawItem.Arguments.ElementCount, drawItem.Arguments.InstanceCount,
                        drawItem.Arguments.FirstElement, drawItem.Arguments.VertexOffset, drawItem.Arguments.FirstInstance);
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        public void Dispose()
        {
            NativeBuilder.Dispose();
        }
    }
}
