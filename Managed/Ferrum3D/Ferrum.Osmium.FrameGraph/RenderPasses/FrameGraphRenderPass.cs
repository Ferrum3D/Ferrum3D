using System.Collections.Generic;
using Ferrum.Osmium.FrameGraph.CommandLists;
using Ferrum.Osmium.FrameGraph.FrameGraph;
using Ferrum.Osmium.FrameGraph.Resources;
using Ferrum.Osmium.GPU.DeviceObjects;

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

        private readonly List<BufferBarrierDesc>[] bufferBarriers =
            new List<BufferBarrierDesc>[(int)BarrierSlot.Count];

        private readonly List<ImageBarrierDesc>[] imageBarriers =
            new List<ImageBarrierDesc>[(int)BarrierSlot.Count];

        internal BufferBarrierDesc[] GetBufferBarriers(BarrierSlot slot)
        {
            return bufferBarriers[(int)slot].ToArray();
        }

        internal ImageBarrierDesc[] GetImageBarriers(BarrierSlot slot)
        {
            return imageBarriers[(int)slot].ToArray();
        }

        internal void AddBarrier(BarrierSlot slot, in BufferBarrierDesc barrier)
        {
            bufferBarriers[(int)slot].Add(barrier);
        }

        internal void AddBarrier(BarrierSlot slot, in ImageBarrierDesc barrier)
        {
            imageBarriers[(int)slot].Add(barrier);
        }

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

        internal void Initialize(FrameGraphBuilder builder)
        {
            SetupDependencies(builder);
        }

        internal void AllocateResources()
        {
            
        }

        internal void Execute(FrameGraphRenderContext context)
        {
            var builder = context.CommandList.NativeBuilder;
            builder.ResourceTransitionBarriers(GetImageBarriers(BarrierSlot.Aliasing), GetBufferBarriers(BarrierSlot.Aliasing));

            RecordCommands(context.CommandList);
        }

        protected abstract void SetupDependencies(FrameGraphBuilder builder);
        protected abstract void RecordCommands(CommandList commandList);

        internal enum BarrierSlot
        {
            Aliasing,
            Count
        }
    }
}
