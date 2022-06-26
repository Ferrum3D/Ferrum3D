using System;
using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU
{
    public static class OsmiumGpuModule
    {
        [DllImport("OsmiumBindings", EntryPoint = "AttachEnvironment")]
        public static extern void AttachEnvironment(IntPtr env);
    }
}
