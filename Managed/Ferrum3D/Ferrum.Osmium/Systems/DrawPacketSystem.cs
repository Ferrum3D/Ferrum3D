using System.Collections.Generic;
using Ferrum.Core.Components;
using Ferrum.Core.Entities;
using Ferrum.Core.Math;
using Ferrum.Osmium.AssetStreaming;
using Ferrum.Osmium.Components;
using Ferrum.Osmium.Drawing;
using Ferrum.Osmium.FrameGraph.CommandLists;
using Ferrum.Osmium.FrameGraph.FrameGraph;

namespace Ferrum.Osmium.Systems
{
    public class DrawPacketSystem : ComponentSystem
    {
        public AssetStreamer AssetStreamer;
        public FrameGraphExecutor FrameGraphExecutor;
        public IReadOnlyList<DrawItem> DrawItems;
        public IReadOnlyList<Vector3F> ItemPositions;
        public IReadOnlyList<ulong> ItemLayers;
        public IReadOnlyList<RenderView> RenderViews;

        private EntityQuery query;

        protected override void OnCreate()
        {
            query = EntityRegistry.CreateQuery();
            query.All = ComponentType.CreateList<RenderMeshComponent, LocalToWorldComponent>();
            AddSubsystem(new AssetStreamingSubsystem(query));
        }

        protected override void OnFrameInit()
        {
            query.Update();
        }

        protected override void OnUpdate()
        {
            for (var i = 0; i < RenderViews.Count; ++i)
            {
                var builder = DrawPacket.Begin(RenderViews[i]);

                for (var j = 0; j < DrawItems.Count; ++j)
                {
                    builder.AddItem(DrawItems[i], ItemPositions[i], ItemLayers[i]);
                }

                RenderViews[i].DrawPacket = builder.End();
            }
        }

        protected override void OnDestroy()
        {
            query.Dispose();
        }
    }
}
