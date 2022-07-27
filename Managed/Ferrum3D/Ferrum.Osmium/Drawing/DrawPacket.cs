using System.Collections.Generic;
using Ferrum.Core.Math;
using Ferrum.Osmium.FrameGraph.CommandLists;

namespace Ferrum.Osmium.Drawing
{
    public sealed class DrawPacket
    {
        public IReadOnlyList<DrawItemSortKey> SortedDrawItems => drawItems;

        private readonly List<DrawItemSortKey> drawItems;

        private DrawPacket(List<DrawItemSortKey> drawItems)
        {
            this.drawItems = drawItems;
        }

        public static Builder Begin(RenderView view, int sizeHint = 0)
        {
            return new Builder(view, sizeHint);
        }

        public class Builder
        {
            private readonly List<DrawItemSortKey> items;
            private readonly RenderView renderView;

            public Builder(RenderView view, int sizeHint)
            {
                items = new List<DrawItemSortKey>(sizeHint);
                renderView = view;
            }

            public void AddItem(in DrawItem item, in Vector3F position, ulong customSortKey = 0)
            {
                var depth = Vector3F.Distance(position, renderView.Position);
                items.Add(new DrawItemSortKey(item, customSortKey, depth));
            }

            public DrawPacket End()
            {
                items.Sort(DrawItemSortKey.GetComparer(renderView.SortSettings));
                return new DrawPacket(items);
            }
        }
    }
}
