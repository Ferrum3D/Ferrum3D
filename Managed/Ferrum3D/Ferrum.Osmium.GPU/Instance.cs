using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU
{
    public class Instance : IDisposable
    {
        private unsafe void* handle;

        public Instance(IntPtr environment, Desc desc, GraphicsApi api)
        {
            unsafe
            {
                AttachEnvironment((void*)environment);
                handle = ConstructNative(ref desc, (int)api);
            }
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        [DllImport("OsmiumBindings", EntryPoint = "AttachEnvironment")]
        private static extern unsafe void AttachEnvironment(void* env);

        [DllImport("OsmiumBindings", EntryPoint = "IInstance_Construct")]
        private static extern unsafe void* ConstructNative(ref Desc desc, int api);

        [DllImport("OsmiumBindings", EntryPoint = "IInstance_Destruct")]
        private static extern unsafe void DestructNative(void* instance);

        [DllImport("OsmiumBindings", EntryPoint = "DetachEnvironment")]
        private static extern void DetachEnvironment();

        private void ReleaseUnmanagedResources()
        {
            unsafe
            {
                DestructNative(handle);
                handle = (void*)0;
            }

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
