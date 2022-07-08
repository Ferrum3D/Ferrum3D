using System.Diagnostics;
using Ferrum.Osmium.FrameGraph.RenderPasses;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.Resources
{
    public sealed class FrameGraphImageResource : FrameGraphResource
    {
        public Image RealImage { get; private set; }
        private readonly Image.Desc desc;

        internal FrameGraphImageResource(Image realImage, int id) : base(id, null)
        {
            RealImage = realImage;
        }

        internal FrameGraphImageResource(Image.Desc desc, int id, FrameGraphRenderPass creator) : base(id, creator)
        {
            this.desc = desc;
        }

        internal override void CreateRealResource(Device device)
        {
            Debug.Assert(IsTransient);
            RealImage = device.CreateImage(desc);
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
