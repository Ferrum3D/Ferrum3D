using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.PipelineStates
{
    public readonly struct RasterizationState
    {
        public static RasterizationState Default = new RasterizationState(CullingModeFlags.None);
        public readonly CullingModeFlags CullMode;
        public readonly PolygonMode PolyMode;
        public readonly bool DepthClampEnabled;
        public readonly bool DepthBiasEnabled;
        public readonly bool RasterDiscardEnabled;

        public RasterizationState(CullingModeFlags cullMode, PolygonMode polyMode = PolygonMode.Fill)
        {
            DepthClampEnabled = false;
            DepthBiasEnabled = false;
            RasterDiscardEnabled = false;
            CullMode = cullMode;
            PolyMode = polyMode;
        }

        public RasterizationState(bool depthClampEnabled, bool depthBiasEnabled, bool rasterDiscardEnabled,
            CullingModeFlags cullMode, PolygonMode polyMode = PolygonMode.Fill)
        {
            DepthClampEnabled = depthClampEnabled;
            DepthBiasEnabled = depthBiasEnabled;
            RasterDiscardEnabled = rasterDiscardEnabled;
            CullMode = cullMode;
            PolyMode = polyMode;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal readonly struct Native
        {
            public readonly CullingModeFlags CullMode;
            public readonly PolygonMode PolyMode;
            public readonly ulong DepthClampDepthBiasRasterDiscardEnabled;

            public Native(RasterizationState state)
            {
                DepthClampDepthBiasRasterDiscardEnabled = 0;
                CullMode = state.CullMode;
                PolyMode = state.PolyMode;

                if (state.DepthClampEnabled)
                {
                    DepthClampDepthBiasRasterDiscardEnabled |= 4;
                }

                if (state.DepthBiasEnabled)
                {
                    DepthClampDepthBiasRasterDiscardEnabled |= 2;
                }

                if (state.RasterDiscardEnabled)
                {
                    DepthClampDepthBiasRasterDiscardEnabled |= 1;
                }
            }
        }
    }
}
