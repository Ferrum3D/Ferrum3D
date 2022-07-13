using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct ImageBarrierDesc
    {
        public readonly IntPtr Image;
        public readonly ImageSubresourceRange SubresourceRange;
        public readonly ResourceState StateBefore;
        public readonly ResourceState StateAfter;

        public ImageBarrierDesc(Image image, ImageSubresourceRange subresourceRange,
            ResourceState stateAfter)
        {
            Image = image.Handle;
            SubresourceRange = subresourceRange;
            StateBefore = ResourceState.Automatic;
            StateAfter = stateAfter;
        }

        public ImageBarrierDesc(Image image, ResourceState stateBefore, ResourceState stateAfter)
        {
            Image = image.Handle;
            SubresourceRange = image.CreateSubresourceRange();
            StateBefore = stateBefore;
            StateAfter = stateAfter;
        }

        public ImageBarrierDesc(Image image, ImageSubresourceRange subresourceRange,
            ResourceState stateBefore, ResourceState stateAfter)
        {
            Image = image.Handle;
            SubresourceRange = subresourceRange;
            StateBefore = stateBefore;
            StateAfter = stateAfter;
        }

        public ImageBarrierDesc(Image image, ResourceState stateAfter, int mipSlice, int mipSliceCount)
        {
            Image = image.Handle;
            SubresourceRange = new ImageSubresourceRange(mipSlice, mipSliceCount, 0, 1, ImageAspectFlags.Color);
            StateBefore = ResourceState.Automatic;
            StateAfter = stateAfter;
        }
    }
}
