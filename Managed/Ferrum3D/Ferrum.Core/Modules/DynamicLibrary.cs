using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Modules
{
    public class DynamicLibrary : IDisposable
    {
        public string Name { get; private set; }
        public IntPtr Handle { get; private set; }

        public static DynamicLibrary FromPath(string name)
        {
            var result = new DynamicLibrary();
            result.LoadFrom(name);
            return result;
        }

        public void LoadFrom(string name)
        {
            Name = name;
            Handle = LoadLibrary(name);
        }

        public void Dispose()
        {
            FreeLibrary(Handle);
        }

        public T GetFunction<T>(string name) where T : Delegate
        {
            var address = GetProcAddress(Handle, "MultiplyByTen");
            return (T)Marshal.GetDelegateForFunctionPointer(address, typeof(T));
        }

        public static void UnloadModule(string moduleName)
        {
            moduleName = moduleName.ToLower();
            var mods = Process.GetCurrentProcess().Modules;
            for (var i = 0; i < mods.Count; i++)
            {
                if (mods[i].ModuleName?.ToLower() == moduleName)
                {
                    FreeLibrary(mods[i].BaseAddress);
                }
            }
        }

        [DllImport("kernel32.dll")]
        private static extern IntPtr LoadLibrary(string path);

        [DllImport("kernel32.dll")]
        private static extern IntPtr GetProcAddress(IntPtr handle, string name);

        [DllImport("kernel32.dll")]
        private static extern bool FreeLibrary(IntPtr handle);
    }
}
