using Ferrum.Osmium.FrameGraph.CommandLists;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.RenderPasses
{
    public class FrameGraphRenderContext
    {
        public FrameGraph.FrameGraph FrameGraph { get; }
        public CommandList CommandList { get; }
        public Device Device { get; }

        public FrameGraphRenderContext(FrameGraph.FrameGraph frameGraph, Device device, CommandList commandList)
        {
            FrameGraph = frameGraph;
            CommandList = commandList;
            Device = device;
        }
    }
}
