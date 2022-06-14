using System.Linq;
using Ferrum.Core.Console;
using Ferrum.Core.Modules;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.WindowSystem;

namespace Ferrum.Samples.Triangle
{
    internal static class Program
    {
        private static void RunExample()
        {
            var instanceDesc = new Instance.Desc("TestApp");
            using var instance = new Instance(Engine.Environment, instanceDesc, GraphicsApi.Vulkan);
            using var adapter = instance.Adapters.First();
            using var device = adapter.CreateDevice();
            using var graphicsQueue = device.GetCommandQueue(CommandQueueClass.Graphics);
            var windowDesc = new Window.Desc(800, 600, "TestApp");
            using var window = device.CreateWindow(windowDesc);
            using var shaderCompiler = device.CreateShaderCompiler();

            while (!window.CloseRequested)
            {
                window.PollEvents();
            }
        }

        private static void Main()
        {
            using var engine = new Engine();
            using var logger = new ConsoleLogger();

            ConsoleLogger.LogMessage("Test unicode. Тестим юникод. 中文考試. Æ ¶ ✅ ♣ ♘");

            RunExample();
        }
    }
}
