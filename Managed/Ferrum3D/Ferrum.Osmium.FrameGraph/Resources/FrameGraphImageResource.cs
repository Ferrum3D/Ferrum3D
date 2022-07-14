using System.Diagnostics;
using Ferrum.Osmium.FrameGraph.RenderPasses;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.Resources
{
    public sealed class FrameGraphImageResource : FrameGraphResource
    {
        public Image RealImage { get; internal set; }
        public readonly Image.Desc Descriptor;

        internal FrameGraphImageResource(Image realImage, ulong id) : base(id, null)
        {
            RealImage = realImage;
        }

        internal FrameGraphImageResource(Image.Desc descriptor, ulong id, FrameGraphRenderPass creator) : base(id, creator)
        {
            Descriptor = descriptor;
        }

        public override void Dispose()
        {
            if (IsTransient)
            {
                RealImage.Dispose();
            }
        }
    }
}
