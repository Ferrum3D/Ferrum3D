using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class Buffer : Resource
    {
        public ulong Size => Descriptor.Size;
        public BindFlags BindFlags => Descriptor.Flags;

        public Desc Descriptor { get; }

        internal Buffer(IntPtr handle, in Desc desc) : base(handle)
        {
            Descriptor = desc;
        }

        public void AllocateMemory(MemoryType memoryType)
        {
            AllocateMemoryNative(Handle, (int)memoryType);
        }

        public void UpdateData<T>(T[] data, int offset)
            where T : unmanaged
        {
            UpdateData(data, (ulong)offset);
        }

        public void UpdateData<T>(T[] data, int offset, int size)
            where T : unmanaged
        {
            UpdateData(data, (ulong)offset, (ulong)size);
        }

        public void UpdateData<T>(T[] data, ulong offset = 0, ulong size = ulong.MaxValue)
            where T : unmanaged
        {
            unsafe
            {
                fixed (void* ptr = data)
                {
                    UpdateDataNative(Handle, new IntPtr(ptr), offset, size);
                }
            }
        }

        public void UpdateData(IntPtr data, ulong offset = 0, ulong size = ulong.MaxValue)
        {
            UpdateDataNative(Handle, data, offset, size);
        }

        [DllImport("OsGPUBindings", EntryPoint = "IBuffer_AllocateMemory")]
        private static extern void AllocateMemoryNative(IntPtr self, int memoryType);

        [DllImport("OsGPUBindings", EntryPoint = "IBuffer_UpdateData")]
        private static extern void UpdateDataNative(IntPtr self, IntPtr data, ulong offset, ulong size);

        [DllImport("OsGPUBindings", EntryPoint = "IBuffer_Destruct")]
        private static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential)]
        public readonly struct Desc
        {
            public readonly ulong Size;
            public readonly BindFlags Flags;

            public Desc(ulong size, BindFlags flags)
            {
                Size = size;
                Flags = flags;
            }
        }
    }
}
