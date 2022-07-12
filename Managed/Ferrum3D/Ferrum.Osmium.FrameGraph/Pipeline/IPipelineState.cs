using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.Pipeline
{
    public interface IPipelineState
    {
        internal void Bind(CommandBuffer.Builder commandBufferBuilder);
    }
}
