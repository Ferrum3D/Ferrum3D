using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Modules
{
    public class DynamicLibrary
    {
        public static void UnloadModule(string moduleName)
        {
            moduleName = moduleName.ToLower();
            foreach (ProcessModule mod in Process.GetCurrentProcess().Modules)
            {
                if (mod.ModuleName?.ToLower() == moduleName)
                {
                    System.Console.WriteLine($"==> Unloading {mod.ModuleName}");
                    FreeLibrary(mod.BaseAddress);
                    System.Console.WriteLine("Unloaded");
                }
            }
        }

        [DllImport("kernel32", SetLastError = true)]
        private static extern bool FreeLibrary(IntPtr hModule);
    }
}
