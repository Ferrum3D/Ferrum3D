using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.PipelineStates
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct TargetColorBlending
    {
        public static TargetColorBlending Default = new(ColorComponentFlags.All, false,
            BlendFactor.One, BlendFactor.Zero, BlendOperation.Add,
            BlendFactor.One, BlendFactor.Zero, BlendOperation.Add);

        public readonly ColorComponentFlags ColorWriteFlags;
        public readonly BlendFactor SourceFactor;
        public readonly BlendFactor DestinationFactor;
        public readonly BlendOperation BlendOp;
        public readonly BlendFactor SourceAlphaFactor;
        public readonly BlendFactor DestinationAlphaFactor;
        public readonly BlendOperation AlphaBlendOp;
        public readonly bool BlendEnabled;

        public TargetColorBlending(ColorComponentFlags colorWriteFlags, bool blendEnabled, BlendFactor sourceFactor,
            BlendFactor destinationFactor, BlendOperation blendOp, BlendFactor sourceAlphaFactor,
            BlendFactor destinationAlphaFactor, BlendOperation alphaBlendOp)
        {
            ColorWriteFlags = colorWriteFlags;
            BlendEnabled = blendEnabled;
            SourceFactor = sourceFactor;
            DestinationFactor = destinationFactor;
            BlendOp = blendOp;
            SourceAlphaFactor = sourceAlphaFactor;
            DestinationAlphaFactor = destinationAlphaFactor;
            AlphaBlendOp = alphaBlendOp;
        }
    }
}
