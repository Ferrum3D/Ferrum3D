namespace Ferrum.Osmium.Drawing
{
    public readonly struct DrawItemSortSettings
    {
        public readonly DrawItemCompareOp DepthCompareOp;
        public readonly DrawItemCompareOp CustomKeyCompareOp;

        public DrawItemSortSettings(DrawItemCompareOp depthCompareOp,
            DrawItemCompareOp customKeyCompareOp = DrawItemCompareOp.None)
        {
            DepthCompareOp = depthCompareOp;
            CustomKeyCompareOp = customKeyCompareOp;
        }

        public bool IsSorting()
        {
            return DepthCompareOp != DrawItemCompareOp.None ||
                   CustomKeyCompareOp != DrawItemCompareOp.None;
        }
    }
}
