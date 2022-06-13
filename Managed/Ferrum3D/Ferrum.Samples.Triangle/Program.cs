using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Ferrum.Core.Console;
using Ferrum.Core.Math;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU;

namespace TestCsProject
{
    internal static class Program
    {
        [DllImport("kernel32", SetLastError = true)]
        static extern bool FreeLibrary(IntPtr hModule);

        public static void UnloadModule(string moduleName)
        {
            moduleName = moduleName.ToLower();
            foreach (ProcessModule mod in Process.GetCurrentProcess().Modules)
            {
                if (mod.ModuleName?.ToLower() == moduleName)
                {
                    Console.WriteLine($"==> Unloading {mod.ModuleName}");
                    FreeLibrary(mod.BaseAddress);
                    Console.WriteLine("Unloaded");
                }
            }
        }

        private static void GraphicsTest()
        {
            var desc = new Instance.Desc("TestApp");
            using var instance = new Instance(Engine.GetEnvironment(), desc, GraphicsApi.Vulkan);
        }

        private static void Main()
        {
            Engine.Init();
            ConsoleLogger.Init();

            ConsoleLogger.LogMessage("Test unicode. Тестим юникод. 中文考試. Æ ¶ ✅ ♣ ♘");

            GraphicsTest();
            UnloadModule("OsmiumBindings.dll");

            ConsoleLogger.Deinit();
            Engine.Deinit();
        }
    }
}
