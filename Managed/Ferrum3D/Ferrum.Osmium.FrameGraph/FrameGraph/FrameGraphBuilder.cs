using Ferrum.Osmium.FrameGraph.RenderPasses;
using Ferrum.Osmium.FrameGraph.Resources;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.FrameGraph
{
    public class FrameGraphBuilder
    {
        private readonly FrameGraph frameGraph;
        private readonly FrameGraphRenderPass renderPass;

        public FrameGraphBuilder(FrameGraph frameGraph, FrameGraphRenderPass renderPass)
        {
            this.frameGraph = frameGraph;
            this.renderPass = renderPass;
        }

        public FrameGraphImageResource CreateImage(in Image.Desc desc)
        {
            var result = new FrameGraphImageResource(desc, frameGraph.Resources.Count, renderPass);
            frameGraph.Resources.Add(result);
            return result;
        }

        public T Read<T>(T resource)
            where T : FrameGraphResource
        {
            resource.ReadFrom(renderPass);
            renderPass.ReadResource(resource);
            return resource;
        }

        public T Write<T>(T resource)
            where T : FrameGraphResource
        {
            resource.WriteFrom(renderPass);
            renderPass.WriteResource(resource);
            return resource;
        }
    }
}
