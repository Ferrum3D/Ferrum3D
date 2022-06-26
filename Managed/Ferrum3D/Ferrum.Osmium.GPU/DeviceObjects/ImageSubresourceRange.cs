using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct ImageSubresourceRange
    {
        public readonly ushort MinMipSlice;
        public readonly ushort MinArraySlice;
        public readonly ushort MipSliceCount;
        public readonly ushort ArraySliceCount;

        public readonly ImageAspectFlags AspectFlags;

        public static readonly ImageSubresourceRange Default =
            new ImageSubresourceRange(0, 0, 1, 1, ImageAspectFlags.RenderTarget);

        public ImageSubresourceRange(ushort minMipSlice, ushort minArraySlice, ushort mipSliceCount,
            ushort arraySliceCount, ImageAspectFlags aspectFlags)
        {
            MinMipSlice = minMipSlice;
            MinArraySlice = minArraySlice;
            MipSliceCount = mipSliceCount;
            ArraySliceCount = arraySliceCount;
            AspectFlags = aspectFlags;
        }
    }
}
