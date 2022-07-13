using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct BufferBarrierDesc
    {
        public readonly IntPtr Buffer;
        public readonly ulong Offset;
        public readonly ulong Size;
        public readonly ResourceState StateBefore;
        public readonly ResourceState StateAfter;

        public BufferBarrierDesc(Buffer buffer, ResourceState stateBefore, ResourceState stateAfter)
        {
            Buffer = buffer.Handle;
            Offset = 0;
            Size = buffer.Size;
            StateBefore = stateBefore;
            StateAfter = stateAfter;
        }

        public BufferBarrierDesc(Buffer buffer, ulong offset, ulong size, ResourceState stateBefore, ResourceState stateAfter)
        {
            Buffer = buffer.Handle;
            Offset = offset;
            Size = size;
            StateBefore = stateBefore;
            StateAfter = stateAfter;
        }
    }
}
