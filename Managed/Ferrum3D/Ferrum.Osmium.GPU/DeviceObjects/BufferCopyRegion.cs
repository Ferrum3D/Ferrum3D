using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public struct BufferCopyRegion
    {
        public readonly ulong Size;
        public readonly uint SourceOffset;
        public readonly uint DestOffset;

        public BufferCopyRegion(uint sourceOffset, uint destOffset, ulong size)
        {
            Size = size;
            SourceOffset = sourceOffset;
            DestOffset = destOffset;
        }

        public BufferCopyRegion(ulong size) : this(0, 0, size)
        {
        }
    }
}
