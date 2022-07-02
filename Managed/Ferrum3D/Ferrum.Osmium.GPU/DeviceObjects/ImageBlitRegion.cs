using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct ImageBlitRegion
    {
        public readonly ImageSubresource Source;
        public readonly ImageSubresource Dest;
        public readonly ImageBounds SourceBounds;
        public readonly ImageBounds DestBounds;

        public ImageBlitRegion(ImageSubresource source, ImageSubresource dest, ImageBounds sourceBounds,
            ImageBounds destBounds)
        {
            Source = source;
            Dest = dest;
            SourceBounds = sourceBounds;
            DestBounds = destBounds;
        }
    }
}
