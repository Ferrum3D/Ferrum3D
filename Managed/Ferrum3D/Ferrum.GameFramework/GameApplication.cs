using Ferrum.Core.Framework;
using Ferrum.Osmium.Assets;
using Ferrum.Osmium.GPU;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.GameFramework
{
    public abstract class GameApplication : ApplicationFramework
    {
        protected GameLevel CurrentLevel { get; }

        protected override bool CloseEventReceived => CurrentLevel.FrameGraphExecutor.Window.CloseRequested;

        public override void Initialize(Desc desc)
        {
            var gpuModule = GetDependency<OsmiumGpuModule>();
            gpuModule.Initialize(new OsmiumGpuModule.Desc(desc.Name, GraphicsApi.Vulkan));
            var assetsModule = GetDependency<OsmiumAssetsModule>();
            assetsModule?.Initialize(new OsmiumAssetsModule.Desc());

        }
    }
}
