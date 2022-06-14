using Ferrum.Core.Console;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU;

namespace Ferrum.Samples.Triangle
{
    internal static class Program
    {
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

            ConsoleLogger.Deinit();
            Engine.Deinit();
        }
    }
}
