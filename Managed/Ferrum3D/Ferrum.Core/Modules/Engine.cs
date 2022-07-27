using System;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Modules
{
    public class Engine : IDisposable
    {
        public static DynamicLibrary CoreModuleLibrary { get; } = new();
        public static IntPtr Environment { get; private set; }

        public Engine()
        {
            unsafe
            {
                if (sizeof(IntPtr) != sizeof(ulong))
                {
                    throw new Exception("The engine can only work on 64 bit systems");
                }
            }

            CoreModuleLibrary.LoadFrom("FeCoreBindings");
            ConstructNative();
            Environment = GetEnvironmentNative();
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        [DllImport("FeCoreBindings", EntryPoint = "Engine_Construct")]
        private static extern void ConstructNative();

        [DllImport("FeCoreBindings", EntryPoint = "Engine_Destruct")]
        private static extern void DestructNative();

        [DllImport("FeCoreBindings", EntryPoint = "Engine_GetEnvironment")]
        private static extern IntPtr GetEnvironmentNative();

        private static void ReleaseUnmanagedResources()
        {
            if (Environment == IntPtr.Zero)
            {
                return;
            }

            DestructNative();
            Environment = IntPtr.Zero;
            CoreModuleLibrary.Dispose();
        }

        ~Engine()
        {
            ReleaseUnmanagedResources();
        }
    }
}
