using System.Collections.Generic;
using Ferrum.Core.Entities;
using Ferrum.Core.Math;
using Ferrum.Osmium.Drawing;
using Ferrum.Osmium.FrameGraph.CommandLists;

namespace Ferrum.Osmium.Systems
{
    public class DrawPacketSystem : ComponentSystem
    {
        public IReadOnlyList<DrawItem> DrawItems;
        public IReadOnlyList<Vector3F> ItemPositions;
        public IReadOnlyList<ulong> ItemLayers;
        public IReadOnlyList<RenderView> RenderViews;

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
    }
}
