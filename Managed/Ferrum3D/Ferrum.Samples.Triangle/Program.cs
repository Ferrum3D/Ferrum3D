using System.Linq;
using Ferrum.Core.Console;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Samples.Triangle
{
    internal static class Program
    {
        private static void RunExample()
        {
            var desc = new Instance.Desc("TestApp");
            using var instance = new Instance(Engine.GetEnvironment(), desc, GraphicsApi.Vulkan);
            using var adapter = instance.Adapters.First();
            using var device = adapter.CreateDevice();
        }

        private static void Main()
        {
            Engine.Init();
            ConsoleLogger.Init();

            ConsoleLogger.LogMessage("Test unicode. Тестим юникод. 中文考試. Æ ¶ ✅ ♣ ♘");

            RunExample();

            ConsoleLogger.Deinit();
            Engine.Deinit();
        }
    }
}
