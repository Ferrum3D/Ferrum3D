using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.CommandLists
{
    public readonly struct VertexBufferSlice
    {
        public readonly Buffer Buffer;
        public readonly ulong ByteOffset;
        public readonly ulong ByteSize;
        public readonly uint ByteStride;
        public ulong VertexCount => ByteSize / ByteStride;

        public VertexBufferSlice(Buffer buffer, ulong byteOffset, ulong byteSize, uint byteStride)
        {
            Buffer = buffer;
            ByteOffset = byteOffset;
            ByteSize = byteSize;
            ByteStride = byteStride;
        }
    }
}
