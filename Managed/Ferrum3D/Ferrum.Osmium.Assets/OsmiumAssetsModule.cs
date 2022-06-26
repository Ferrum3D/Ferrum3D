using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.Assets
{
    public static class OsmiumAssetsModule
    {
        [DllImport("OsmiumAssetsBindings", EntryPoint = "AttachEnvironment")]
        public static extern void AttachEnvironment(IntPtr environment);
    }
}
