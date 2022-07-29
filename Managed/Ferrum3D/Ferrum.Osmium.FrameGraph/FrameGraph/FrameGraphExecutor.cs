using System;
using System.Linq;
using Ferrum.Core.Framework;
using Ferrum.Osmium.GPU;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.WindowSystem;

namespace Ferrum.Osmium.FrameGraph.FrameGraph
{
    public sealed class FrameGraphExecutor : IDisposable
    {
        public FrameGraph FrameGraph { get; }

        public Device Device { get; }
        public Window Window { get; }
        public SwapChain SwapChain { get; }
        public CommandQueue GraphicsQueue { get; }
        public CommandQueue TransferQueue { get; }

        private readonly Instance instance;
        private readonly Adapter adapter;

        public FrameGraphExecutor()
        {
            var gpuModule = OsmiumGpuModule.Factory.Instance;

            instance = gpuModule.CreateInstance();
            adapter = instance.Adapters.First();
            Device = adapter.CreateDevice();

            GraphicsQueue = Device.GetCommandQueue(CommandQueueClass.Graphics);
            TransferQueue = Device.GetCommandQueue(CommandQueueClass.Transfer);

            var desc = ApplicationFramework.Instance.Descriptor;
            var windowDesc = new Window.Desc(desc.WindowWidth, desc.WindowHeight, desc.Name);
            Window = Device.CreateWindow(windowDesc);

            var swapChainDesc = new SwapChain.Desc(Window, GraphicsQueue);
            SwapChain = Device.CreateSwapChain(swapChainDesc);

            FrameGraph = new FrameGraph(Device);
        }

        public void Tick()
        {
            FrameGraph.Compile();
        }

        public void Dispose()
        {
            FrameGraph.Dispose();
            Window.Dispose();
            SwapChain.Dispose();
            GraphicsQueue.Dispose();
            TransferQueue.Dispose();
            Device.Dispose();
            adapter.Dispose();
            instance.Dispose();
        }
    }
}
