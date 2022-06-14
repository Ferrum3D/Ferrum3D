using System;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Modules
{
    public class Engine : IDisposable
    {
        public static IntPtr Environment { get; private set; }

        public Engine()
        {
            InitNative();
            Environment = GetEnvironmentNative();
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        [DllImport("FeCoreBindings", EntryPoint = "InitEngine")]
        private static extern void InitNative();

        private static void ReleaseUnmanagedResources()
        {
            DeinitNative();
            Environment = IntPtr.Zero;
        }

        [DllImport("FeCoreBindings", EntryPoint = "DeinitEngine")]
        private static extern void DeinitNative();

        [DllImport("FeCoreBindings", EntryPoint = "GetEnvironment")]
        private static extern IntPtr GetEnvironmentNative();

        ~Engine()
        {
            ReleaseUnmanagedResources();
        }
    }
}
