using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Core.Containers
{
    public abstract class ByteBuffer : UnmanagedObject
    {
        public IntPtr DataPointer => Handle == IntPtr.Zero ? IntPtr.Zero : DataNative(Handle);

        protected ByteBuffer(long size) : this(ConstructNative((ulong)size))
        {
        }

        protected ByteBuffer(int size) : this(ConstructNative((ulong)size))
        {
        }

        protected ByteBuffer(IntPtr handle) : base(handle)
        {
        }

        [DllImport("FeCoreBindings", EntryPoint = "IByteBuffer_Construct")]
        private static extern IntPtr ConstructNative(ulong size);

        [DllImport("FeCoreBindings", EntryPoint = "IByteBuffer_Size")]
        private protected static extern ulong SizeNative(IntPtr self);

        [DllImport("FeCoreBindings", EntryPoint = "IByteBuffer_Data")]
        private static extern IntPtr DataNative(IntPtr self);
        
        [DllImport("FeCoreBindings", EntryPoint = "IByteBuffer_CopyTo")]
        private protected static extern void CopyToNative(IntPtr self, IntPtr dest);

        [DllImport("FeCoreBindings", EntryPoint = "IByteBuffer_Destruct")]
        private protected static extern void DestructNative(IntPtr self);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }
    }
}
