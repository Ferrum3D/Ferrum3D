using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Containers;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.Descriptors
{
    public class DescriptorHeap : UnmanagedObject
    {
        internal DescriptorHeap(IntPtr handle) : base(handle)
        {
        }

        public DescriptorTable AllocateDescriptorTable(params DescriptorDesc[] descriptors)
        {
            unsafe
            {
                fixed (DescriptorDesc* ptr = descriptors)
                {
                    var handle = AllocateDescriptorTableNative(Handle, new IntPtr(ptr), (uint)descriptors.Length);
                    return handle == IntPtr.Zero
                        ? null
                        : new DescriptorTable(handle);
                }
            }
        }

        public void Reset()
        {
            ResetNative(Handle);
        }

        [DllImport("OsGPUBindings", EntryPoint = "IDescriptorHeap_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "IDescriptorHeap_Reset")]
        private static extern void ResetNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "IDescriptorHeap_AllocateDescriptorTable")]
        private static extern IntPtr AllocateDescriptorTableNative(IntPtr handle, IntPtr d, uint count);

        protected override void ReleaseUnmanagedResources()
        {
            DestructNative(Handle);
        }

        internal readonly struct DescNative
        {
            public readonly ByteBuffer.Native Sizes;
            public readonly uint MaxTables;

            public DescNative(in Desc desc)
            {
                Sizes = new NativeArray<DescriptorSize>(desc.Sizes).Detach();
                MaxTables = (uint)desc.MaxTables;
            }
        }

        public readonly struct Desc
        {
            public readonly List<DescriptorSize> Sizes;
            public readonly int MaxTables;

            private Desc(List<DescriptorSize> sizes, int maxTables)
            {
                Sizes = sizes;
                MaxTables = maxTables;
            }

            public Desc(IEnumerable<DescriptorSize> sizes, int maxTables)
            {
                Sizes = sizes.ToList();
                MaxTables = maxTables;
            }

            public Desc WithSizes(params DescriptorSize[] sizes)
            {
                return new Desc(sizes.ToList(), MaxTables);
            }

            public Desc WithMaxTables(int maxTables)
            {
                return new Desc(Sizes, maxTables);
            }
        }
    }
}
