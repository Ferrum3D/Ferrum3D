namespace Ferrum.Osmium.FrameGraph.CommandLists
{
    public readonly struct DrawArguments
    {
        public readonly DrawType DrawType;
        public readonly uint FirstElement;
        public readonly uint ElementCount;
        public readonly uint FirstInstance;
        public readonly uint InstanceCount;
        public readonly int VertexOffset;

        private DrawArguments(DrawType drawType, uint elementCount, uint firstElement, int vertexOffset, uint firstInstance,
            uint instanceCount)
        {
            DrawType = drawType;
            ElementCount = elementCount;
            FirstElement = firstElement;
            VertexOffset = vertexOffset;
            FirstInstance = firstInstance;
            InstanceCount = instanceCount;
        }

        public static DrawArguments CreateLinear(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance)
        {
            return new DrawArguments(DrawType.Linear, vertexCount, firstVertex, 0, firstInstance, instanceCount);
        }

        public static DrawArguments CreateIndexed(uint indexCount, uint instanceCount, uint firstIndex, int vertexOffset,
            uint firstInstance)
        {
            return new DrawArguments(DrawType.Indexed, indexCount, firstIndex, vertexOffset, firstInstance, instanceCount);
        }
    }
}
