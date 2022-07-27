using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.Pipeline
{
    public class GraphicsPipelineState : IPipelineState
    {
        private GraphicsPipeline pipeline;

        public GraphicsPipelineState(Device device)
        {
        }

        public void Dispose()
        {
            pipeline.Dispose();
        }

        void IPipelineState.Bind(CommandBuffer.Builder commandBufferBuilder)
        {
            commandBufferBuilder.BindGraphicsPipeline(pipeline);
        }
    }
}
