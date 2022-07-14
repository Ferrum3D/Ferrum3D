using System;
using System.Collections.Generic;
using System.Linq;
using Ferrum.Core.Containers;
using Ferrum.Osmium.FrameGraph.RenderPasses;
using Ferrum.Osmium.FrameGraph.Resources;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.FrameGraph
{
    public sealed class FrameGraph : IDisposable
    {
        internal readonly DisposableList<FrameGraphResource> Resources = new();
        public FrameGraphImageResource RenderTarget { get; private set; }
        public FrameGraphImageResource DepthStencil { get; private set; }

        private readonly List<FrameGraphRenderPass> renderPasses = new();

        public void SetRenderTarget(Image renderTarget)
        {
            var resource = new FrameGraphImageResource(renderTarget, (ulong)Resources.Count);
            Resources.Add(resource);
            RenderTarget = resource;
        }

        public void SetDepthStencil(Image depthStencil)
        {
            var resource = new FrameGraphImageResource(depthStencil, (ulong)Resources.Count);
            Resources.Add(resource);
            DepthStencil = resource;
        }

        public void RegisterRenderPass(FrameGraphRenderPass renderPass)
        {
            renderPasses.Add(renderPass);
            var builder = new FrameGraphBuilder(this, renderPass);
            renderPass.Initialize(builder);
        }

        public void Compile()
        {
            InitializeReferenceCounters();
            CullRenderPasses();
        }

        public void Dispose()
        {
            Resources.Dispose();
        }

        private void CullRenderPasses()
        {
            var unreferencedResources = new Stack<FrameGraphResource>();
            for (var i = 0; i < Resources.Count; ++i)
            {
                if (Resources[i].ReferenceCount == 0 && Resources[i].IsTransient)
                {
                    unreferencedResources.Push(Resources[i]);
                }
            }

            while (unreferencedResources.Any())
            {
                var resource = unreferencedResources.Pop();
                TryCullRenderPass(resource.Creator, unreferencedResources);

                for (var i = 0; i < resource.Writers.Count; i++)
                {
                    TryCullRenderPass(resource.Writers[i], unreferencedResources);
                }
            }
        }

        private static void TryCullRenderPass(FrameGraphRenderPass pass,
            Stack<FrameGraphResource> unreferencedResources)
        {
            if (pass.RemoveReference() != 0 || pass.CullImmune)
            {
                return;
            }

            for (var i = 0; i < pass.Reads.Count; ++i)
            {
                if (pass.Reads[i].RemoveReference() == 0 && pass.Reads[i].IsTransient)
                {
                    unreferencedResources.Push(pass.Reads[i]);
                }
            }
        }

        private void InitializeReferenceCounters()
        {
            for (var i = 0; i < renderPasses.Count; ++i)
            {
                renderPasses[i].CalculateInitialReferenceCount();
            }

            for (var i = 0; i < Resources.Count; ++i)
            {
                Resources[i].CalculateInitialReferenceCount();
            }
        }
    }
}
