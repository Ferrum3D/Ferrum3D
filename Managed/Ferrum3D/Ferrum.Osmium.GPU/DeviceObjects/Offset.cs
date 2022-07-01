using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.DeviceObjects
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct Offset
    {
        public static readonly Offset Zero;
        public readonly long X;
        public readonly long Y;
        public readonly long Z;

        public Offset(long x, long y, long z = 0)
        {
            X = x;
            Y = y;
            Z = z;
        }
    }
}
