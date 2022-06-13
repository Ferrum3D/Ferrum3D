using System;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Modules
{
    public static class Engine
    {
        [DllImport("FeCoreBindings", EntryPoint = "InitEngine")]
        public static extern void Init();

        [DllImport("FeCoreBindings", EntryPoint = "DeinitEngine")]
        public static extern void Deinit();

        [DllImport("FeCoreBindings", EntryPoint = "GetEnvironment")]
        private static extern unsafe void* GetEnvironmentNative();

        public static IntPtr GetEnvironment()
        {
            unsafe
            {
                return new IntPtr(GetEnvironmentNative());
            }
        }
    }
}
