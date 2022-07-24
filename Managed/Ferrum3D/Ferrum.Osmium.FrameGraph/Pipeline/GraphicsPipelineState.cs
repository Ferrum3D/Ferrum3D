using System;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.Pipeline
{
    public class GraphicsPipelineState : IPipelineState
    {
        private GraphicsPipeline pipeline;

        public GraphicsPipelineState(Device device)
        {
            
        }

        void IPipelineState.Bind(CommandBuffer.Builder commandBufferBuilder)
        {
            commandBufferBuilder.BindGraphicsPipeline(pipeline);
        }

        public void Dispose()
        {
            pipeline.Dispose();
        }
    }
}
