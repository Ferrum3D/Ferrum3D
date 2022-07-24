using System;
using System.Runtime.InteropServices;
using Buffer = Ferrum.Osmium.GPU.DeviceObjects.Buffer;

namespace Ferrum.Osmium.GPU.Descriptors
{
    public readonly struct DescriptorWriteBuffer
    {
        public readonly Buffer Buffer;
        public readonly int Binding;
        public readonly int ArrayIndex;
        public readonly int Offset;
        public readonly int Range;

        public DescriptorWriteBuffer(Buffer buffer, int binding, int range = -1, int offset = 0, int arrayIndex = 0)
        {
            Buffer = buffer;
            Binding = binding;
            ArrayIndex = arrayIndex;
            Offset = offset;
            Range = range;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct Native
        {
            public readonly IntPtr Buffer;
            public readonly uint Binding;
            public readonly uint ArrayIndex;
            public readonly uint Offset;
            public readonly uint Range;

            public Native(DescriptorWriteBuffer write)
            {
                Buffer = write.Buffer?.Handle ?? IntPtr.Zero;
                Binding = (uint)write.Binding;
                ArrayIndex = (uint)write.ArrayIndex;
                Offset = (uint)write.Offset;
                Range = write.Range == -1 ? uint.MaxValue : (uint)write.Range;
            }
        }
    }
}
