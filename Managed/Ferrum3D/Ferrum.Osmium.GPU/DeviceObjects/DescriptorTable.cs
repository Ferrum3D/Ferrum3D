using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
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

    public class DescriptorTable : UnmanagedObject
    {
        public DescriptorTable(IntPtr handle) : base(handle)
        {
        }

        public void Update(int binding, Buffer buffer)
        {
            Update(new DescriptorWriteBuffer(buffer, binding));
        }

        public void Update(DescriptorWriteBuffer descriptorWriteBuffer)
        {
            var nativeWrite = new DescriptorWriteBuffer.Native(descriptorWriteBuffer);
            UpdateNative(Handle, ref nativeWrite);
        }

        [DllImport("OsmiumBindings", EntryPoint = "IDescriptorTable_Destruct")]
        private static extern void DestructNative(IntPtr handle);

        [DllImport("OsmiumBindings", EntryPoint = "IDescriptorTable_Update")]
        private static extern void UpdateNative(IntPtr handle, ref DescriptorWriteBuffer.Native writeBuffer);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }
    }
}
