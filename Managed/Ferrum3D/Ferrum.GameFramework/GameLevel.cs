using System;
using Ferrum.Core.Entities;
using Ferrum.Osmium.AssetStreaming;
using Ferrum.Osmium.Drawing;
using Ferrum.Osmium.FrameGraph.CommandLists;
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
                RenderViews = new[] { MainCameraView },
                DrawItems = Array.Empty<DrawItem>(),
                AssetStreamer = new AssetStreamer(),
                FrameGraphExecutor = FrameGraphExecutor
            };

            EntityWorld.RegisterSystem(drawPacketSystem);
        }

        public void Begin()
        {
            FrameGraphExecutor.Begin();
        }

        public void Tick()
        {
            FrameGraphExecutor.Execute();
        }

        public void Dispose()
        {
            EntityWorld.Dispose();
            FrameGraphExecutor.Dispose();
        }
    }
}
