using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    public sealed class Instance : IDisposable
    {
        public IReadOnlyList<Adapter> Adapters => adapters;
        private IntPtr handle;
        private readonly Adapter[] adapters;

        public Instance(Desc desc, GraphicsApi api)
        {
            handle = ConstructNative(ref desc, (int)api);
            GetAdaptersNative(handle, null, out var adapterCount);
            var a = new IntPtr[adapterCount];
            GetAdaptersNative(handle, a, out _);
            adapters = a.Select(x => new Adapter(x)).ToArray();
        }

        public void Dispose()
        {
            for (var i = 0; i < Adapters.Count; i++)
            {
                var adapter = Adapters[i];
                adapter.Dispose();
            }

            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        [DllImport("OsmiumBindings", EntryPoint = "IInstance_Construct")]
        private static extern IntPtr ConstructNative(ref Desc desc, int api);

        [DllImport("OsmiumBindings", EntryPoint = "IInstance_Destruct")]
        private static extern void DestructNative(IntPtr self);

        [DllImport("OsmiumBindings", EntryPoint = "DetachEnvironment")]
        private static extern void DetachEnvironment();

        [DllImport("OsmiumBindings", EntryPoint = "IInstance_GetAdapters")]
        private static extern void GetAdaptersNative(IntPtr self, IntPtr[] adapters, out int size);

        private void ReleaseUnmanagedResources()
        {
            if (handle == IntPtr.Zero)
            {
                return;
            }

            DestructNative(handle);
            handle = IntPtr.Zero;

            DetachEnvironment();
            DynamicLibrary.UnloadModule("OsmiumBindings.dll");
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
