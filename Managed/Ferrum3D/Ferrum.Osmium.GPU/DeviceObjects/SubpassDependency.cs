using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct SubpassDependency
    {
        public static readonly SubpassDependency Default = new SubpassDependency(uint.MaxValue,
            PipelineStageFlags.ColorAttachmentOutput, ResourceState.Common, 0,
            PipelineStageFlags.ColorAttachmentOutput,
            ResourceState.RenderTarget);

        public readonly uint SourceSubpassIndex;
        public readonly PipelineStageFlags SourcePipelineStage;
        public readonly ResourceState SourceState;

        public readonly uint DestinationSubpassIndex;
        public readonly PipelineStageFlags DestinationPipelineStage;
        public readonly ResourceState DestinationState;

        public SubpassDependency(uint sourceSubpassIndex, PipelineStageFlags sourcePipelineStage,
            ResourceState sourceState, uint destinationSubpassIndex, PipelineStageFlags destinationPipelineStage,
            ResourceState destinationState)
        {
            SourceSubpassIndex = sourceSubpassIndex;
            SourcePipelineStage = sourcePipelineStage;
            SourceState = sourceState;
            DestinationSubpassIndex = destinationSubpassIndex;
            DestinationPipelineStage = destinationPipelineStage;
            DestinationState = destinationState;
        }
    }
}
