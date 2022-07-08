using System.Collections.Generic;
using Ferrum.Osmium.FrameGraph.FrameGraph;
using Ferrum.Osmium.FrameGraph.Resources;

namespace Ferrum.Osmium.FrameGraph.RenderPasses
{
    public abstract class FrameGraphRenderPass
    {
        internal IReadOnlyList<FrameGraphResource> Creates => creates;
        internal IReadOnlyList<FrameGraphResource> Writes => writes;
        internal IReadOnlyList<FrameGraphResource> Reads => reads;

        internal int ReferenceCount;
        protected internal virtual bool CullImmune => false;

        private readonly List<FrameGraphResource> creates = new();
        private readonly List<FrameGraphResource> writes = new();
        private readonly List<FrameGraphResource> reads = new();

        internal int RemoveReference()
        {
            return ReferenceCount == 0 ? 0 : --ReferenceCount;
        }

        internal void CalculateInitialReferenceCount()
        {
            ReferenceCount = writes.Count + reads.Count;
        }

        internal void CreateResource(FrameGraphResource resource)
        {
            creates.Add(resource);
        }

        internal void ReadResource(FrameGraphResource resource)
        {
            reads.Add(resource);
        }

        internal void WriteResource(FrameGraphResource resource)
        {
            writes.Add(resource);
        }

        protected internal abstract void Initialize(FrameGraphBuilder builder);
    }
}
