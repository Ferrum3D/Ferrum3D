using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public sealed class Instance : UnmanagedObject
    {
        public IReadOnlyList<Adapter> Adapters => adapters;
        private readonly Adapter[] adapters;

        public Instance(IntPtr handle) : base(handle)
        {
            GetAdaptersNative(handle, null, out var adapterCount);
            var a = new IntPtr[adapterCount];
            GetAdaptersNative(handle, a, out _);
            adapters = a.Select(x => new Adapter(x)).ToArray();
        }

        [DllImport("OsGPUBindings", EntryPoint = "IInstance_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("OsGPUBindings", EntryPoint = "IInstance_GetAdapters")]
        private static extern void GetAdaptersNative(IntPtr self, IntPtr[] adapters, out int size);

        protected override void ReleaseUnmanagedResources()
        {
            for (var i = 0; i < Adapters.Count; i++)
            {
                var adapter = Adapters[i];
                adapter.Dispose();
            }

            DestructNative(Handle);
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct Desc
        {
            public readonly string ApplicationName;

            public Desc(string applicationName)
            {
                ApplicationName = applicationName;
            }
        }

        ~Instance()
        {
            ReleaseUnmanagedResources();
        }
    }
}
