using System;
using Ferrum.Osmium.GPU.DeviceObjects;

namespace Ferrum.Osmium.FrameGraph.Pipeline
{
    public interface IPipelineState : IDisposable
    {
        internal void Bind(CommandBuffer.Builder commandBufferBuilder);
    }
}
