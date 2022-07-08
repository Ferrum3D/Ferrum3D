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

        public Delegate GetFunction(string name, Type delegateType)
        {
            var address = GetProcAddress(Handle, name);
            return Marshal.GetDelegateForFunctionPointer(address, delegateType);
        }

        public T GetFunction<T>(string name) where T : Delegate
        {
            var address = GetProcAddress(Handle, name);
            return Marshal.GetDelegateForFunctionPointer<T>(address);
        }

        public static IntPtr GetLoadedModule(string moduleName)
        {
            moduleName = moduleName.ToLower();
            var mods = Process.GetCurrentProcess().Modules;
            for (var i = 0; i < mods.Count; i++)
            {
                if (mods[i].ModuleName?.ToLower() == moduleName)
                {
                    return mods[i].BaseAddress;
                }
            }

            return IntPtr.Zero;
        }

        [DllImport("kernel32.dll")]
        public static extern bool FreeLibrary(IntPtr handle);

        [DllImport("kernel32.dll")]
        private static extern IntPtr LoadLibrary(string path);

        [DllImport("kernel32.dll")]
        private static extern IntPtr GetProcAddress(IntPtr handle, string name);
    }
}
