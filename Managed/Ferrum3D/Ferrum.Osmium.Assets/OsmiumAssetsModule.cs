using System;
using System.Runtime.InteropServices;
using Ferrum.Core.Modules;

namespace Ferrum.Osmium.Assets
{
    public class OsmiumAssetsModule : IDisposable
    {
        [DllImport("OsmiumAssetsBindings", EntryPoint = "AttachEnvironment")]
        private static extern void AttachEnvironment(IntPtr environment);

        [DllImport("OsmiumAssetsBindings", EntryPoint = "DetachEnvironment")]
        private static extern void DetachEnvironment();

        public OsmiumAssetsModule(IntPtr environment)
        {
            AttachEnvironment(environment);
        }

        public void Dispose()
        {
            DetachEnvironment();
            DynamicLibrary.UnloadModule("OsmiumAssetsBindings.dll");
        }
    }
}
