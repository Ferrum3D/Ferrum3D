using System.Runtime.InteropServices;
using Ferrum.Core.Math;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct ClearValueDesc
    {
        public readonly Color ColorValue;
        public readonly float DepthValue;
        public readonly uint StencilValue;
        public readonly bool IsDepth;

        private ClearValueDesc(Color colorValue, float depthValue, uint stencilValue, bool isDepth)
        {
            ColorValue = colorValue;
            DepthValue = depthValue;
            StencilValue = stencilValue;
            IsDepth = isDepth;
        }

        public static ClearValueDesc CreateColorValue(Color color)
        {
            return new ClearValueDesc(color, 1.0f, 0, false);
        }

        public static ClearValueDesc CreateDepthStencilValue(float depth = 1.0f, uint stencil = 0)
        {
            return new ClearValueDesc(Color.Zero, depth, stencil, true);
        }
    }
}
