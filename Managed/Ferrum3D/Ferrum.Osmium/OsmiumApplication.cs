using System;
using System.Collections.Generic;
using System.Linq;
using Ferrum.Core.Framework;
using Ferrum.Core.Utils;
using Ferrum.Osmium.Assets;
using Ferrum.Osmium.Drawing;
using Ferrum.Osmium.GPU;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.WindowSystem;
using Ferrum.Osmium.Systems;

namespace Ferrum.Osmium
{
    public abstract class OsmiumApplication : ApplicationFramework
    {
        protected Instance Instance { get; private set; }
        protected Adapter Adapter { get; private set; }
        protected Device Device { get; private set; }
        protected Window Window { get; private set; }
        protected SwapChain SwapChain { get; private set; }
        protected CommandQueue GraphicsQueue { get; private set; }

        protected override bool CloseEventReceived => Window.CloseRequested;

        public override void Initialize(Desc desc)
        {
            base.Initialize(desc);
            var gpuModule = GetDependency<OsmiumGpuModule>();
            gpuModule.Initialize(new OsmiumGpuModule.Desc(desc.Name, GraphicsApi.Vulkan));
            var assetsModule = GetDependency<OsmiumAssetsModule>();
            assetsModule?.Initialize(new OsmiumAssetsModule.Desc());

            Instance = gpuModule.CreateInstance();
            Adapter = Instance.Adapters.First();
            Device = Adapter.CreateDevice();

            GraphicsQueue = Device.GetCommandQueue(CommandQueueClass.Graphics);

            var windowDesc = new Window.Desc(desc.WindowWidth, desc.WindowHeight, desc.Name);
            Window = Device.CreateWindow(windowDesc);

            var swapChainDesc = new SwapChain.Desc(Window, GraphicsQueue);
            SwapChain = Device.CreateSwapChain(swapChainDesc);
        }

        protected override void GetFrameworkDependencies(ICollection<IFrameworkFactory> dependencies)
        {
            dependencies.Add(new OsmiumGpuModule.Factory());
            if (Descriptor.AssetDirectory != null)
            {
                dependencies.Add(new OsmiumAssetsModule.Factory());
            }
        }

        protected override void BeginFrame()
        {
            Window.PollEvents();
        }

        protected override void OnExit()
        {
            Device.WaitIdle();
            this.DisposeFields();
        }
    }
}
