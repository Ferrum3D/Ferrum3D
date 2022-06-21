using System.Runtime.InteropServices;
using Ferrum.Core.Math;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public struct ClearValueDesc
    {
        public readonly Color Color;

        public ClearValueDesc(Color color)
        {
            Color = color;
        }
    }
}
