using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public struct BufferImageCopyRegion
    {
        public readonly Offset ImageOffset;
        public readonly Size ImageSize;
        public readonly ImageSubresource ImageSubresource;
        public readonly ulong BufferOffset;

        public BufferImageCopyRegion(Offset imageOffset, Size imageSize, ImageSubresource imageSubresource,
            ulong bufferOffset)
        {
            ImageOffset = imageOffset;
            ImageSize = imageSize;
            ImageSubresource = imageSubresource;
            BufferOffset = bufferOffset;
        }

        public BufferImageCopyRegion(Size imageSize)
            : this(Offset.Zero, imageSize, new ImageSubresource(), 0)
        {
        }
    }
}
