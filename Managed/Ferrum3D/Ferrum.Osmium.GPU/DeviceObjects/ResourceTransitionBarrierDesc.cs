using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public readonly struct ResourceTransitionBarrierDesc
    {
        public readonly Image Image;
        public readonly ImageSubresourceRange SubresourceRange;
        public readonly ResourceState StateBefore;
        public readonly ResourceState StateAfter;

        public ResourceTransitionBarrierDesc(Image image, ImageSubresourceRange subresourceRange,
            ResourceState stateAfter)
        {
            Image = image;
            SubresourceRange = subresourceRange;
            StateBefore = ResourceState.Automatic;
            StateAfter = stateAfter;
        }

        public ResourceTransitionBarrierDesc(Image image, ImageSubresourceRange subresourceRange,
            ResourceState stateBefore, ResourceState stateAfter)
        {
            Image = image;
            SubresourceRange = subresourceRange;
            StateBefore = stateBefore;
            StateAfter = stateAfter;
        }

        public ResourceTransitionBarrierDesc(Image image, ResourceState stateAfter, int mipSlice, int mipSliceCount)
        {
            Image = image;
            SubresourceRange = new ImageSubresourceRange(mipSlice, mipSliceCount, 0, 1, ImageAspectFlags.Color);
            StateBefore = ResourceState.Automatic;
            StateAfter = stateAfter;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct Native
        {
            public readonly IntPtr Image;
            public readonly ImageSubresourceRange SubresourceRange;
            public readonly ResourceState StateBefore;
            public readonly ResourceState StateAfter;

            public Native(ResourceTransitionBarrierDesc desc)
            {
                Image = desc.Image.Handle;
                SubresourceRange = desc.SubresourceRange;
                StateBefore = desc.StateBefore;
                StateAfter = desc.StateAfter;
            }
        }
    }
}
