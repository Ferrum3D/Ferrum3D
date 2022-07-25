using System.Collections.Generic;
using Ferrum.Osmium.FrameGraph.CommandLists;

namespace Ferrum.Osmium.Drawing
{
    public struct DrawItemSortKey
    {
        public readonly DrawItem DrawItem;
        public readonly ulong SortKey;
        public readonly float Depth;

        public DrawItemSortKey(DrawItem drawItem, ulong sortKey, float depth)
        {
            DrawItem = drawItem;
            SortKey = sortKey;
            Depth = depth;
        }

        public static IComparer<DrawItemSortKey> GetComparer(in DrawItemSortSettings settings)
        {
            return new RelationalComparer(in settings);
        }

        private sealed class RelationalComparer : IComparer<DrawItemSortKey>
        {
            private readonly DrawItemSortSettings sortSettings;

            public RelationalComparer(in DrawItemSortSettings settings)
            {
                sortSettings = settings;
            }

            public int Compare(DrawItemSortKey x, DrawItemSortKey y)
            {
                var keyCompare = x.SortKey.CompareTo(y.SortKey);
                if (keyCompare != 0)
                {
                    switch (sortSettings.CustomKeyCompareOp)
                    {
                        case DrawItemCompareOp.Less:
                            return keyCompare;
                        case DrawItemCompareOp.Greater:
                            return -keyCompare;
                    }
                }

                var depthCompare = x.Depth.CompareTo(y.Depth);
                switch (sortSettings.DepthCompareOp)
                {
                    case DrawItemCompareOp.Less:
                        return depthCompare;
                    case DrawItemCompareOp.Greater:
                        return -depthCompare;
                }

                return 0;
            }
        }
    }
}
