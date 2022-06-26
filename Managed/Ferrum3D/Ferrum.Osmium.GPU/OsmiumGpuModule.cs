using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.GPU
{
    public class OsmiumGpuModule : IDisposable
    {
        [DllImport("OsmiumBindings", EntryPoint = "AttachEnvironment")]
        private static extern void AttachEnvironment(IntPtr env);
        
        [DllImport("OsmiumBindings", EntryPoint = "DetachEnvironment")]
        private static extern void DetachEnvironment();

        public OsmiumGpuModule(IntPtr environment)
        {
            AttachEnvironment(environment);
        }

        public void Dispose()
        {
            DetachEnvironment();
            DynamicLibrary.UnloadModule("OsmiumBindings.dll");
        }
    }
}
