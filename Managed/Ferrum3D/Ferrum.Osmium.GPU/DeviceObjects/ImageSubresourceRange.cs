using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct ImageSubresourceRange
    {
        public static ImageSubresourceRange Default =
            new ImageSubresourceRange(0, 1, 0, 1, ImageAspectFlags.Color);

        public readonly ushort MinMipSlice;
        public readonly ushort MinArraySlice;
        public readonly ushort MipSliceCount;
        public readonly ushort ArraySliceCount;

        public readonly ImageAspectFlags AspectFlags;

        public ImageSubresourceRange(int minMipSlice, int mipSliceCount, int minArraySlice,
            int arraySliceCount, ImageAspectFlags aspectFlags)
        {
            MinMipSlice = (ushort)minMipSlice;
            MipSliceCount = (ushort)mipSliceCount;
            MinArraySlice = (ushort)minArraySlice;
            ArraySliceCount = (ushort)arraySliceCount;
            AspectFlags = aspectFlags;
        }
    }
}
