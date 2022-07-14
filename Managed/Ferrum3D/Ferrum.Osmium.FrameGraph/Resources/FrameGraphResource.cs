using System;
using System.Collections.Generic;
using Ferrum.Osmium.FrameGraph.RenderPasses;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.Resources
{
    public abstract class FrameGraphResource : IDisposable
    {
        internal ulong Id { get; }
        internal FrameGraphRenderPass Creator { get; }
        internal bool IsTransient => Creator != null;

        internal IReadOnlyList<FrameGraphRenderPass> Readers => readers;
        internal IReadOnlyList<FrameGraphRenderPass> Writers => writers;

        internal int ReferenceCount { get; set; }

        private readonly List<FrameGraphRenderPass> readers = new();
        private readonly List<FrameGraphRenderPass> writers = new();

        protected FrameGraphResource(ulong id, FrameGraphRenderPass creator)
        {
            Id = id;
            Creator = creator;
        }

        internal void ReadFrom(FrameGraphRenderPass renderPass)
        {
            readers.Add(renderPass);
        }

        internal void WriteFrom(FrameGraphRenderPass renderPass)
        {
            writers.Add(renderPass);
        }

        public abstract void Dispose();

        internal int RemoveReference()
        {
            return ReferenceCount == 0 ? 0 : --ReferenceCount;
        }

        internal void CalculateInitialReferenceCount()
        {
            ReferenceCount = Readers.Count;
        }
    }
}
