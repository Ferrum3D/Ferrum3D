using System.Collections.Generic;
using Ferrum.Osmium.FrameGraph.Pipeline;
using Ferrum.Osmium.GPU.Common;

namespace Ferrum.Osmium.FrameGraph.CommandLists
{
    public struct DrawItem
    {
        public readonly IPipelineState PipelineState;
        public readonly IndexBufferSlice Indices;
        public readonly DrawArguments Arguments;
        public readonly List<VertexBufferSlice> Vertices;
        // public readonly ShaderResources ShaderResources;
        public readonly Viewport Viewport;
        public readonly Scissor Scissor;
    }
}
