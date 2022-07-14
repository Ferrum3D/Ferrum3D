using Ferrum.Osmium.FrameGraph.RenderPasses;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.Resources
{
    public class FrameGraphBufferResource : FrameGraphResource
    {
        public Buffer RealBuffer { get; internal set; }
        public readonly Buffer.Desc Descriptor;

        internal FrameGraphBufferResource(Buffer realBuffer, ulong id) : base(id, null)
        {
            RealBuffer = realBuffer;
        }

        internal FrameGraphBufferResource(Buffer.Desc descriptor, ulong id, FrameGraphRenderPass creator) : base(id, creator)
        {
            Descriptor = descriptor;
        }

        public override void Dispose()
        {
            if (IsTransient)
            {
                RealBuffer.Dispose();
            }
        }
    }
}
