using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public readonly struct ResourceTransitionBarrierDesc
    {
        public readonly Image Image;
        public readonly ImageSubresourceRange SubresourceRange;
        public readonly ResourceState StateAfter;

        public ResourceTransitionBarrierDesc(Image image, ImageSubresourceRange subresourceRange, ResourceState stateAfter)
        {
            Image = image;
            SubresourceRange = subresourceRange;
            StateAfter = stateAfter;
        }

        public ResourceTransitionBarrierDesc(Image image, ResourceState stateAfter)
        {
            Image = image;
            SubresourceRange = ImageSubresourceRange.Default;
            StateAfter = stateAfter;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct Native
        {
            public readonly IntPtr Image;
            public readonly ImageSubresourceRange SubresourceRange;
            public readonly ResourceState StateAfter;

            public Native(ResourceTransitionBarrierDesc desc)
            {
                Image = desc.Image.Handle;
                SubresourceRange = desc.SubresourceRange;
                StateAfter = desc.StateAfter;
            }
        }
    }
}
