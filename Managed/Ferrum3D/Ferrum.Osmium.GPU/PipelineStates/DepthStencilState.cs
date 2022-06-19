using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.PipelineStates
{
    public readonly struct DepthStencilState
    {
        public readonly CompareOp DepthCompareOp;
        public readonly bool DepthTestEnabled;
        public readonly bool DepthWriteEnabled;

        public DepthStencilState(bool depthTestEnabled, bool depthWriteEnabled, CompareOp depthCompareOp)
        {
            DepthTestEnabled = depthTestEnabled;
            DepthWriteEnabled = depthWriteEnabled;
            DepthCompareOp = depthCompareOp;
        }

        public static DepthStencilState Default = new DepthStencilState(false, false, CompareOp.Less);

        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct Native
        {
            public readonly CompareOp DepthCompareOp;
            public readonly uint DepthTestWriteEnabled;

            public Native(DepthStencilState state)
            {
                DepthTestWriteEnabled = 0;
                DepthCompareOp = state.DepthCompareOp;
                if (state.DepthTestEnabled)
                {
                    DepthTestWriteEnabled |= 2;
                }

                if (state.DepthWriteEnabled)
                {
                    DepthTestWriteEnabled |= 1;
                }
            }
        }
    }
}
