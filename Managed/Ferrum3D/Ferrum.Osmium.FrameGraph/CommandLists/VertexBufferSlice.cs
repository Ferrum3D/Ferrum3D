using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.CommandLists
{
    public readonly struct VertexBufferSlice
    {
        public readonly Buffer Buffer;
        public readonly ulong ByteOffset;
        public readonly ulong ByteSize;
        public readonly int ByteStride;
        public ulong VertexCount => ByteSize / (uint)ByteStride;

        public static readonly VertexBufferSlice Empty = new(null, 0, 0, 0);

        public VertexBufferSlice(Buffer buffer, ulong byteOffset, ulong byteSize, int byteStride)
        {
            Buffer = buffer;
            ByteOffset = byteOffset;
            ByteSize = byteSize;
            ByteStride = byteStride;
        }
    }
}
