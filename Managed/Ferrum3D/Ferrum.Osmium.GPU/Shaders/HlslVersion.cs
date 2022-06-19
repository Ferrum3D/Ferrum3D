using System.Runtime.InteropServices;

namespace Ferrum.Osmium.GPU.Shaders
{
    [StructLayout(LayoutKind.Sequential)]
    public readonly struct HlslVersion
    {
        public static readonly HlslVersion V6 = new HlslVersion(6, 1);
        public readonly uint Major;
        public readonly uint Minor;

        public HlslVersion(uint major, uint minor)
        {
            Major = major;
            Minor = minor;
        }
    }
}
