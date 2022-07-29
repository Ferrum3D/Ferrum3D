using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.CommandLists
{
    public readonly struct IndexBufferSlice
    {
        public readonly Buffer Buffer;
        public readonly ulong ByteOffset;
        public readonly ulong ByteSize;
        public ulong IndexCount => ByteSize / sizeof(uint);

        public static readonly IndexBufferSlice Empty = new(null, 0, 0);

        public IndexBufferSlice(Buffer buffer, ulong byteOffset, ulong byteSize)
        {
            Buffer = buffer;
            ByteOffset = byteOffset;
            ByteSize = byteSize;
        }
    }
}
