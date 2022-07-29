using System.Collections.Generic;
using Ferrum.Core.Entities;
using Ferrum.Core.EventBus;
using Ferrum.Core.Framework;
using Ferrum.Core.Utils;
using Ferrum.Osmium.Assets;
using Ferrum.Osmium.GPU;
using Ferrum.Osmium.GPU.DeviceObjects;
using Ferrum.Osmium.GPU.WindowSystem;

namespace Ferrum.GameFramework
{
    public abstract class GameApplication : ApplicationFramework
    {
        protected GameLevel CurrentLevel { get; private set; }
        protected EntityRegistry EntityRegistry => CurrentLevel.EntityRegistry;

        protected override bool CloseEventReceived => CurrentLevel.FrameGraphExecutor.Window.CloseRequested;

        public override void Initialize(Desc desc)
        {
            base.Initialize(desc);
            var gpuModule = GetDependency<OsmiumGpuModule>();
            gpuModule.Initialize(new OsmiumGpuModule.Desc(desc.Name, GraphicsApi.Vulkan));
            var assetsModule = GetDependency<OsmiumAssetsModule>();
            assetsModule?.Initialize(new OsmiumAssetsModule.Desc());

            CurrentLevel = new GameLevel();
        }

        protected override void GetFrameworkDependencies(ICollection<IFrameworkFactory> dependencies)
        {
            dependencies.Add(new OsmiumGpuModule.Factory());
            if (Descriptor.AssetDirectory != null)
            {
                dependencies.Add(new OsmiumAssetsModule.Factory());
            }
        }

        protected override void Tick(FrameEventArgs frameEventArgs)
        {
            CurrentLevel.Tick();
        }

        protected override void BeginFrame()
        {
            CurrentLevel.Begin();
        }

        protected override void OnExit()
        {
            CurrentLevel.Dispose();
        }
    }
}
