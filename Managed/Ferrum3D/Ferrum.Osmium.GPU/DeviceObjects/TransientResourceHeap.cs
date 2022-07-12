using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public class TransientResourceHeap : UnmanagedObject
    {
        internal TransientResourceHeap(IntPtr handle) : base(handle)
        {
        }

        public void Allocate()
        {
            AllocateNative(Handle);
        }

        public bool TryCreateImage(TransientImageDesc desc, out Image image, out AllocationStats stats)
        {
            var handle = CreateImageNative(Handle, ref desc, out stats);
            if (handle == IntPtr.Zero)
            {
                image = null;
                return false;
            }

            image = new Image(handle, desc.Descriptor);
            return true;
        }

        public bool TryCreateBuffer(TransientBufferDesc desc, out Buffer buffer, out AllocationStats stats)
        {
            var handle = CreateBufferNative(Handle, ref desc, out stats);
            if (handle == IntPtr.Zero)
            {
                buffer = null;
                return false;
            }

            buffer = new Buffer(handle);
            return true;
        }

        public void ReleaseImage(ulong resourceId)
        {
            ReleaseImageNative(Handle, resourceId);
        }

        public void ReleaseBuffer(ulong resourceId)
        {
            ReleaseBufferNative(Handle, resourceId);
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public readonly struct AllocationStats
        {
            public readonly ulong MinOffset;
            public readonly ulong MaxOffset;
        }

        [DllImport("OsGPUBindings", EntryPoint = "ITransientResourceHeap_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "ITransientResourceHeap_Allocate")]
        private static extern void AllocateNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "ITransientResourceHeap_CreateImage")]
        private static extern IntPtr CreateImageNative(IntPtr self, ref TransientImageDesc desc, out AllocationStats stats);

        [DllImport("OsGPUBindings", EntryPoint = "ITransientResourceHeap_CreateBuffer")]
        private static extern IntPtr CreateBufferNative(IntPtr self, ref TransientBufferDesc desc, out AllocationStats stats);

        [DllImport("OsGPUBindings", EntryPoint = "ITransientResourceHeap_ReleaseImage")]
        private static extern IntPtr ReleaseImageNative(IntPtr self, ulong resourceId);

        [DllImport("OsGPUBindings", EntryPoint = "ITransientResourceHeap_ReleaseBuffer")]
        private static extern IntPtr ReleaseBufferNative(IntPtr self, ulong resourceId);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential)]
        public readonly struct Desc
        {
            public readonly ulong HeapSize;
            public readonly ulong Alignment;
            public readonly int CacheSize;
            public readonly TransientResourceType Type;

            public Desc(ulong heapSize, ulong alignment, int cacheSize, TransientResourceType type)
            {
                HeapSize = heapSize;
                Alignment = alignment;
                CacheSize = cacheSize;
                Type = type;
            }

            public Desc(TransientResourceType type)
                : this(512 * 1024, 256, 256, type)
            {
            }
        }
    }
}
