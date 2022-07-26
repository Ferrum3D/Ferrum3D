using System;
using Ferrum.Core.Entities;
using Ferrum.Osmium.Drawing;
using Ferrum.Osmium.FrameGraph.FrameGraph;
using Ferrum.Osmium.Systems;

namespace Ferrum.GameFramework
{
    public sealed class GameLevel : IDisposable
    {
        public World EntityWorld { get; }
        public EntityRegistry EntityRegistry => EntityWorld.EntityRegistry;

        public FrameGraphExecutor FrameGraphExecutor { get; }
        public RenderView MainCameraView { get; }

        public GameLevel()
        {
            EntityWorld = new World();
            FrameGraphExecutor = new FrameGraphExecutor();
            MainCameraView = RenderView.CreateCamera(FrameGraphExecutor.SwapChain.AspectRatio);

            var drawPacketSystem = new DrawPacketSystem
            {
                RenderViews = new[] { MainCameraView }
            };

            EntityWorld.RegisterSystem(drawPacketSystem);
        }

        public void Dispose()
        {
            EntityWorld.Dispose();
            FrameGraphExecutor.Dispose();
        }
    }
}
