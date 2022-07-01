using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Ferrum.Core.Modules
{
    public static class DynamicLibrary
    {
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

        [DllImport("kernel32", SetLastError = true)]
        private static extern bool FreeLibrary(IntPtr hModule);
    }
}
